/*
 *  DCC.cpp
 * 
 *  This file is part of CommandStation.
 *
 *  CommandStation is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  CommandStation is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CommandStation.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "DCC.h"

#include "../CommInterface/CommManager.h"

DCC::DCC(uint8_t numDevices, Hardware settings) {
  this->hdw = settings;            
  this->numDevices = numDevices;      

  packetQueue.clear();
  
  // Allocate memory for the speed table
  speedTable = (Speed *)calloc(numDevices+1, sizeof(Speed));
  for (int i = 0; i <= numDevices+1; i++)
  {
    speedTable[i].cab = 0;
    speedTable[i].forward = true;
    speedTable[i].speed = 0;
  }
}

void DCC::schedulePacket(const uint8_t buffer[], uint8_t byteCount, 
  uint8_t repeats, uint16_t identifier, PacketType type, uint16_t address) {
  if(byteCount >= kPacketMaxSize) return; // allow for checksum
  
  Packet newPacket;

  uint8_t checksum=0;
  for (int b=0; b<byteCount; b++) {
    checksum ^= buffer[b];
    newPacket.payload[b] = buffer[b];
  }
  newPacket.payload[byteCount] = checksum;
  newPacket.length = byteCount+1;
  newPacket.repeats = repeats;
  newPacket.transmitID = identifier;
  newPacket.type = type;
  newPacket.address = address;

  const Packet pushPacket = newPacket;
  noInterrupts();
  packetQueue.push(pushPacket);
  interrupts();   
}

void DCC::updateSpeed() {
  if (packetQueue.count() > 0) return;  // Only update speed if queue is empty

  for (; nextDev < numDevices; nextDev++) {
    if (speedTable[nextDev].cab > 0) {
      setThrottleResponse response;
      setThrottle(nextDev, speedTable[nextDev].cab, speedTable[nextDev].speed, 
        speedTable[nextDev].forward, response);
      nextDev++;
      return;
    }
  }
  for (nextDev = 0; nextDev < numDevices; nextDev++) {
    if (speedTable[nextDev].cab > 0) {
      setThrottleResponse response;
      setThrottle(nextDev, speedTable[nextDev].cab, speedTable[nextDev].speed, 
        speedTable[nextDev].forward, response);
      nextDev++;
      return;
    }
  }
}

int DCC::setThrottle(uint8_t slot, uint16_t addr, uint8_t speed, 
  bool direction, setThrottleResponse& response) {
  
  uint8_t b[5];     // Packet payload. Save space for checksum byte
  uint8_t nB = 0;   // Counter for number of bytes in the packet
  uint16_t railcomAddr = 0;  // For detecting the railcom instruction type

  if((slot < 1) || (slot > numDevices))
    return ERR_OUT_OF_RANGE;

  if(addr > 127) {
    b[nB++] = highByte(addr) | 0xC0;    // convert address to packet format
    railcomAddr = (highByte(addr) | 0xC0) << 8;
  } 

  b[nB++]=lowByte(addr);
  railcomAddr |= lowByte(addr);
  b[nB++]=0x3F;   // 128-step speed control byte
  if(speed>=0)
    // max speed is 126, but speed codes range from 2-127 
    // (0=stop, 1=emergency stop)
    b[nB++]=speed+(speed>0)+direction*128;   
  else{
    b[nB++]=1;
    speed=0;
  }

  incrementCounterID();
  schedulePacket(b, nB, 0, counterID, kThrottleType, railcomAddr);

  speedTable[slot].speed = speed;
  speedTable[slot].cab = addr;
  speedTable[slot].forward = direction; 

  response.device = addr;
  response.direction = direction;
  response.speed = speed;
  response.transactionID = counterID;

  return ERR_OK;
}

int DCC::setFunction(uint16_t addr, uint8_t byte1, 
  setFunctionResponse& response) {
  
  uint8_t b[4];     // Packet payload. Save space for checksum byte
  uint8_t nB = 0;   // Counter for number of bytes in the packet
  uint16_t railcomAddr = 0;  // For detecting the railcom instruction type

  if(addr > 127) {
    b[nB++] = highByte(addr) | 0xC0;    // convert address to packet format
    railcomAddr = (highByte(addr) | 0xC0) << 8;
  }

  b[nB++] = lowByte(addr);
  railcomAddr |= lowByte(addr);

  b[nB++] = (byte1 | 0x80) & 0xBF;

  incrementCounterID();
  // Repeat the packet four times (one plus 3 repeats)
  schedulePacket(b, nB, 3, counterID, kFunctionType, railcomAddr);  

  response.transactionID = counterID;

  return ERR_OK;
}

int DCC::setFunction(uint16_t addr, uint8_t byte1, uint8_t byte2, 
  setFunctionResponse& response) {
  
  uint8_t b[4];     // Packet payload. Save space for checksum byte
  uint8_t nB = 0;   // Counter for number of bytes in the packet
  uint16_t railcomAddr = 0;  // For detecting the railcom instruction type

  if(addr > 127) {
    b[nB++] = highByte(addr) | 0xC0;    // convert address to packet format
    railcomAddr = (highByte(addr) | 0xC0) << 8;
  }

  b[nB++] = lowByte(addr);
  railcomAddr |= lowByte(addr);

  // for safety this guarantees that first byte will either be 0xDE 
  // (for F13-F20) or 0xDF (for F21-F28)
  b[nB++]=(byte1 | 0xDE) & 0xDF;     
  b[nB++]=byte2;
  
  incrementCounterID();
  // Repeat the packet four times (one plus 3 repeats)
  schedulePacket(b, nB, 3, counterID, kFunctionType, railcomAddr);  

  response.transactionID = counterID;

  return ERR_OK;
}

int DCC::setAccessory(uint16_t addr, uint8_t number, bool activate, 
  setAccessoryResponse& response) {
  
  uint8_t b[3];     // Packet payload. Save space for checksum byte
  uint16_t railcomAddr = 0;  // For detecting the railcom instruction type

  // first byte is of the form 10AAAAAA, where AAAAAA represent 6 least 
  // signifcant bits of accessory address
  b[0] = (addr % 64) + 128;           
  // second byte is of the form 1AAACDDD, where C should be 1, and the least 
  // significant D represent activate/deactivate                                
  b[1] = ((((addr / 64) % 8) << 4) + (number % 4 << 1) + activate % 2) ^ 0xF8;      
  railcomAddr = (b[0] << 8) | b[1];

  incrementCounterID();
  // Repeat the packet four times (one plus 3 repeats)
  schedulePacket(b, 2, 3, counterID, kAccessoryType, railcomAddr); 

  response.transactionID = counterID;

  return ERR_OK;
}

int DCC::writeCVByteMain(uint16_t addr, uint16_t cv, uint8_t bValue, 
  writeCVByteMainResponse& response, void (*POMCallback)(RailcomPOMResponse)) {
  
  uint8_t b[6];     // Packet payload. Save space for checksum byte
  uint8_t nB = 0;   // Counter for number of bytes in the packet
  uint16_t railcomAddr = 0;  // For detecting the railcom instruction type

  cv--;   // actual CV addresses are cv-1 (0-1023)

  if(addr > 127) {
    b[nB++] = highByte(addr) | 0xC0;    // convert address to packet format
    railcomAddr = (highByte(addr) | 0xC0) << 8;
  } 

  b[nB++] = lowByte(addr);
  railcomAddr |= lowByte(addr);

  // any CV>1023 will become modulus(1024) due to bit-mask of 0x03
  b[nB++] = 0xEC + (highByte(cv) & 0x03);   
  b[nB++] = lowByte(cv);
  b[nB++] = bValue;

  hdw.railcom.config_setPOMResponseCallback(POMCallback);

  incrementCounterID();
  // Repeat the packet four times (one plus 3 repeats)
  schedulePacket(b, nB, 3, counterID, kPOMByteWriteType, railcomAddr); 

  response.transactionID = counterID;

  return ERR_OK;
}

int DCC::writeCVBitMain(uint16_t addr, uint16_t cv, uint8_t bNum, 
  uint8_t bValue, writeCVBitMainResponse& response,
  void (*POMCallback)(RailcomPOMResponse)) {
  
  uint8_t b[6];     // Packet payload. Save space for checksum byte
  uint8_t nB = 0;   // Counter for number of bytes in the packet
  uint16_t railcomAddr = 0;  // For detecting the railcom instruction type

  cv--;   // actual CV addresses are cv-1 (0-1023)

  bValue = bValue % 2;
  bNum = bNum % 8;

  if(addr > 127) {
    b[nB++] = highByte(addr) | 0xC0;    // convert address to packet format
    railcomAddr = (highByte(addr) | 0xC0) << 8;
  } 

  b[nB++] = lowByte(addr);
  railcomAddr |= lowByte(addr);

  // any CV>1023 will become modulus(1024) due to bit-mask of 0x03
  b[nB++] = 0xE8 + (highByte(cv) & 0x03);   
  b[nB++] = lowByte(cv);
  b[nB++] = 0xF0 + (bValue * 8) + bNum;

  hdw.railcom.config_setPOMResponseCallback(POMCallback);

  incrementCounterID();
  schedulePacket(b, nB, 4, counterID, kPOMBitWriteType, railcomAddr);

  response.transactionID = counterID;

  return ERR_OK;
}

int DCC::writeCVByte(uint16_t cv, uint8_t bValue, uint16_t callback, 
  uint16_t callbackSub, void(*callbackFunc)(serviceModeResponse)) {
  
  // If we're in the middle of a read/write or if there's not room in the queue.
  if(ackNeeded != 0 || inVerify || (packetQueue.count() > (kQueueSize - 8))) 
    return ERR_BUSY;
  
  uint8_t bWrite[4];

  hdw.setBaseCurrent();

  cv--;       // actual CV addresses are cv-1 (0-1023)

  // any CV>1023 will become modulus(1024) due to bit-mask of 0x03
  bWrite[0]=0x7C+(highByte(cv)&0x03);                     
  bWrite[1]=lowByte(cv);
  bWrite[2]=bValue;

  incrementCounterID();
  // NMRA recommends starting with 3 reset packets (one plus two repeats)
  schedulePacket(kResetPacket, 2, 2, counterID, kResetType, 0);           
  // NMRA recommends 5 verify packets (one plue 4 repeats)
  schedulePacket(bWrite, 3, 4, counterID, kSrvcByteWriteType, 0);          
  // NMRA recommends 6 write or reset packets for decoder recovery time 
  // (one plus 5 repeats)      
  schedulePacket(bWrite, 3, 5, counterID, kSrvcByteWriteType, 0);                
  
  incrementCounterID();
   // set-up to re-verify entire byte
  bWrite[0]=0x74+(highByte(cv)&0x03);                
  // NMRA recommends starting with 3 reset packets (one plus two repeats)    
  schedulePacket(kResetPacket, 2, 2, counterID, kResetType, 0);        
  // We send 2 packets before looking for an ack (one plus one repeat)   
  schedulePacket(bWrite, 3, 1, counterID, kSrvcByteWriteType, 0);                

  incrementCounterID();
  // NMRA recommends 5 verify packets - we sent 2, here are three more 
  // (one plus two repeats)
  schedulePacket(bWrite, 3, 2, counterID, kSrvcByteWriteType, 0);           
  // NMRA recommends 6 write or reset packets for decoder recovery time 
  // (one plus five repeats)     
  schedulePacket(bWrite, 3, 5, counterID, kSrvcByteWriteType, 0);                
  
  ackPacketID[0] = counterID;
  
  incrementCounterID();
  // Final reset packet (and decoder begins to respond) (one plus no repeats)
  schedulePacket(kResetPacket, 2, 0, counterID, kResetType, 0);           

  inVerify = true;

  cvState.type = WRITECV;
  cvState.callback = callback;
  cvState.callbackSub = callbackSub;
  cvState.cv = cv+1;
  cvState.cvValue = bValue;

  backToIdle = false;

  cvResponse = callbackFunc;

  return ERR_OK;
}


int DCC::writeCVBit(uint16_t cv, uint8_t bNum, uint8_t bValue, 
  uint16_t callback, uint16_t callbackSub, 
  void(*callbackFunc)(serviceModeResponse)) {
  
  // If we're in the middle of a read/write or if there's not room in the queue.
  if(ackNeeded != 0 || inVerify || (packetQueue.count() > (kQueueSize - 8))) 
    return ERR_BUSY;
  
  byte bWrite[4];

  hdw.setBaseCurrent();

  cv--;         // actual CV addresses are cv-1 (0-1023)
  bValue=bValue%2;
  bNum=bNum%8;

  // any CV>1023 will become modulus(1024) due to bit-mask of 0x03
  bWrite[0]=0x78+(highByte(cv)&0x03);   
  bWrite[1]=lowByte(cv);
  bWrite[2]=0xF0+bValue*8+bNum;

  incrementCounterID();
  // NMRA recommends starting with 3 reset packets (one plus two repeats)
  schedulePacket(kResetPacket, 2, 2, counterID, kResetType, 0);          
  // NMRA recommends 5 verify packets (one plue 4 repeats) 
  schedulePacket(bWrite, 3, 4, counterID, kSrvcBitWriteType, 0);                
  // NMRA recommends 6 write or reset packets for decoder recovery time 
  // (one plus 5 repeats)
  schedulePacket(bWrite, 3, 5, counterID, kSrvcBitWriteType, 0);                
  
  incrementCounterID();
  bitClear(bWrite[2],4);  // change instruction code from Write to Verify
  // NMRA recommends starting with 3 reset packets (one plus two repeats)
  schedulePacket(kResetPacket, 2, 2, counterID, kResetType, 0); 
  // We send 2 packets before looking for an ack (one plus one repeat)
  schedulePacket(bWrite, 3, 1, counterID, kSrvcBitWriteType, 0);                

  incrementCounterID();
  // NMRA recommends 5 verify packets - we already sent 2, here are three more 
  // (one plus two repeats)
  schedulePacket(bWrite, 3, 2, counterID, kSrvcBitWriteType, 0);              
  // NMRA recommends 6 write or reset packets for decoder recovery time 
  // (one plus five repeats)  
  schedulePacket(bWrite, 3, 5, counterID, kSrvcBitWriteType, 0);                
  
  ackPacketID[0] = counterID;
  
  incrementCounterID();
  // Final reset packet (and decoder begins to respond) (one plus no repeats)
  schedulePacket(kResetPacket, 2, 0, counterID, kResetType, 0);           

  inVerify = true;

  cvState.type = WRITECVBIT;
  cvState.callback = callback;
  cvState.callbackSub = callbackSub;
  cvState.cv = cv+1;
  cvState.cvBitNum = bNum;
  cvState.cvValue = bValue;

  backToIdle = false;

  cvResponse = callbackFunc;

  return ERR_OK;
}


int DCC::readCV(uint16_t cv, uint16_t callback, uint16_t callbackSub, 
  void(*callbackFunc)(serviceModeResponse)) {
  
  // If we're in the middle of a read/write or if there's not room in the queue.
  if(ackNeeded != 0 || inVerify || (packetQueue.count() > (kQueueSize - 25))) 
    return ERR_BUSY;
  
  uint8_t bRead[4];

  hdw.setBaseCurrent();

  cv--;    // actual CV addresses are cv-1 (0-1023)

  // any CV>1023 will become modulus(1024) due to bit-mask of 0x03
  bRead[0]=0x78+(highByte(cv)&0x03);            
  bRead[1]=lowByte(cv);
  
  // Queue up all unique packets required for the CV read. 
  for(int i=0;i<8;i++) {                                  
    bRead[2]=0xE8+i;

    incrementCounterID();
    // NMRA recommends starting with 3 reset packets (one plue two repeats)
    schedulePacket(kResetPacket, 2, 2, counterID, kResetType, 0);       
    // We send 2 packets before looking for an ack (one plus one repeat)
    schedulePacket(bRead, 3, 1, counterID, kSrvcReadType, 0);             

    incrementCounterID();
    // NMRA recommends 5 verify packets - we already sent 2, here are three more 
    // (one plus two repeats)
    schedulePacket(bRead, 3, 2, counterID, kSrvcReadType, 0);             

    ackPacketID[i] = counterID;

    incrementCounterID();
    // NMRA recommends 1 reset packet after checking for an ACK 
    // (one and no repeats)
    schedulePacket(kResetPacket, 2, 0, counterID, kResetType, 0);       
  }

  ackNeeded = 0b11111111;
  
  cvState.type = READCV;
  cvState.callback = callback;
  cvState.callbackSub = callbackSub;
  cvState.cv = cv+1;

  verifyPayload[0]=0x74+(highByte(cv)&0x03);  // set-up to re-verify entire byte
  verifyPayload[1]=lowByte(cv);
  // verifyPayload[2] and verifyPayload[3] get set in checkAck when verifying 

  backToIdle = false;

  cvResponse = callbackFunc;

  return ERR_OK;
}

void DCC::checkAck() {
  float currentMilliamps = hdw.getMilliamps();
  if(!inVerify && (ackNeeded == 0)) return;

  if(!inVerify) {
    uint16_t currentAckID;
    for (uint8_t i = 0; i < 8; i++)
    {
      if(!bitRead(ackNeeded, i)) continue;  // We don't need an ack on this bit

      currentAckID = ackPacketID[i];
      uint16_t compareID = transmitID;
      if(currentAckID == compareID) {
        if((currentMilliamps - hdw.getBaseCurrent()) > kACKThreshold) {
          bitSet(ackBuffer, i);       // We got an ack on this bit
          bitClear(ackNeeded, i);     // We no longer need an ack on this bit

          // Fast-forward to the next packet set
          noInterrupts();
          transmitRepeats = 0;    // Stop transmitting current packet
          while(packetQueue.peek().transmitID == currentAckID) {
            packetQueue.pop();  // Pop off all packets with the the same ID
          }
          interrupts();
        }
      }
      // TODO(davidcutting42@gmail.com): check for wraparound
      else if(compareID > currentAckID || backToIdle) {    
        bitClear(ackBuffer, i);  // We didn't get an ack on this bit (timeout)
        bitClear(ackNeeded, i);  // We no longer need an ack on this bit
      }

      if(ackNeeded == 0) {        // If we've now gotten all the ACKs we need 
        if(cvState.type == READCV) {
          verifyPayload[2] = ackBuffer;   // Set up the verifyPayload for verify
        
          incrementCounterID();               
          // Load 3 reset packets
          schedulePacket(kResetPacket, 2, 2, counterID, kResetType, 0); 
          // Load 5 verify packets
          schedulePacket(verifyPayload, 3, 4, counterID, kSrvcReadType, 0);     
          // Load 1 Reset packet
          schedulePacket(kResetPacket, 2, 0, counterID, kResetType, 0);       

          ackPacketID[0] = counterID;

          incrementCounterID();
          // We need one additional packet with incremented counter so ACK 
          // completes and doesn't hang in checkAck()
          schedulePacket(kResetPacket, 2, 0, counterID, kResetType, 0);   

          inVerify = true;
          backToIdle = false;
        }
        break;
      }
    }    
  }
  else {
    uint16_t compareID = transmitID;
    if(ackPacketID[0] == compareID) {
      if((currentMilliamps - hdw.getBaseCurrent()) > kACKThreshold) {
        inVerify = false;
        if(cvState.type == READCV)
          cvState.cvValue = ackBuffer;
        cvResponse(cvState);

        // Fast-forward to the next packet set
        noInterrupts();
        transmitRepeats = 0;  // Stop transmitting current packet
        while(packetQueue.peek().transmitID == ackPacketID[0]) {
          packetQueue.pop();  // Pop off all packets with the the same ID
        }
        interrupts();
      }
    }
    // TODO(davidcutting42@gmail.com): check for wraparound
    else if(compareID > ackPacketID[0] || backToIdle) {
      inVerify = false;
      
      cvState.cvValue = -1;
      cvResponse(cvState);
    }
  }
}