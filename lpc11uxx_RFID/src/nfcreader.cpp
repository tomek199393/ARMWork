
#include "armcmx.h"
#include "USARTSerial.h"

#include "main.h"

extern const byte IizukaKey_b[] = {
  0xBB, 0x63, 0x45, 0x74, 0x55, 0x79, 0x4B };
extern const byte factory_a[] = {
  0xaa, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

#if defined(USE_PN532)
PN532 nfcreader(Wire, PN532::I2C_ADDRESS, NFC_IRQ, NFC_RSTPD);
const byte NFCPolling[] = {
  NFC::BAUDTYPE_212K_F,
  NFC::BAUDTYPE_106K_A,
};
#elif defined (USE_SL030)
#define SL030_TAGDETECT PIO0_17
#define SL030_WAKEUP PIO0_16
StrongLink_I2C nfcreader(StrongLink_I2C::SL030_ADDRESS, SL030_TAGDETECT, SL030_WAKEUP);
#endif

uint8 getIDInfo(ISO14443Card & card, const byte authkey[8], char data[64]) {
  switch (card.type) {
    case NFC::CARDTYPE_MIFARE:
      if ( get_MifareBlock(card, authkey, data) == 0 ) {
        Serial.println("Unknown Mifare, not an ID.");
        card.clear();
        return 0;
      }
      break;
#ifdef USE_PN532
    case NFC::CARDTYPE_FELICA_212K:
      if ( get_FCFBlock(card, data) == 0 ) {
        Serial.println("Unknown FeliCa, not an FCF.");
        card.clear();
        return 0;
      }
      break;
#endif
    default:
      Serial.println("Not supported as ID card.");
      card.clear();
      return 0;
	}
	return 1;
}


uint8 putIDInfo(ISO14443Card & card, const byte authkey[8], char data[64]) {
  uint8 res;
  uint8 tmp[4];
  
  if ( card.type != NFC::CARDTYPE_MIFARE )
    return 0;
#if defined(USE_PN532)  
  if ( nfcreader.mifare_AuthenticateBlock(4, authkey) 
  && nfcreader.getCommandResponse(&res) && res == 0) {
#elif defined(USE_SL030)
  if ( nfcreader.auth_sector(1, authkey) ) {
#endif
    Serial.println("Authenticated.");
#if defined(USE_PN532)  
    if ( nfcreader.mifare_ReadAccessConditions(7>>2, tmp) ) {
#elif defined(USE_SL030)
    if ( nfcreader.read_access_condition(7>>2, tmp) ) {
#endif
      /*
      acc = ACCESSBITS(tmp);
      Serial.println("Original trailer cond.");
      Serial.printBytes(tmp, 16);
      Serial.println();
      Serial.println(acc, BIN);
      acc &= ~TRAILERBITS(B111);
      acc |=  TRAILERBITS(B011);
      acc &= ~(DATABLOCKBITS(B111, 0) | DATABLOCKBITS(B111, 1) | DATABLOCKBITS(B111, 2));
      acc |=  ( DATABLOCKBITS(B110, 0) | DATABLOCKBITS(B110, 1) | DATABLOCKBITS(B110, 2));
      if ( nfcreader.mifare_WriteAccessConditions(7>>2, acc, factory_a+1, IizukaKey_b+1) ) {
        Serial.println("Succeeded to write trailer block.");
        acc = nfcreader.mifare_ReadAccessConditions(7>>2, tmp);
      } else {
        Serial.println("Trailer block write failed.");
        return 0;
      }
  */    
#if defined(USE_PN532)  
      if ( nfcreader.mifare_WriteBlock(4, data) )
#elif defined(USE_SL030)
      if ( nfcreader.write_block(4, (uint8*) data) )
#endif
        Serial.println("Write to data block succeeded.");
      else {
        Serial.println("Data block write failed.");
        return 0;
      }
      
    } else {
      Serial.println("Read trailer block failed.");
      return 0;
    }
    return 1;
  } else {
    Serial.println("Auth block failed.");
  }
  return 0;
}
#if defined (USE_PN532)
uint8 get_FCFBlock(ISO14443Card & card, char data[64]) {
  word syscode = 0x00FE;
  int len;
  byte c;

  // Polling command, with system code request.
  len = nfcreader.felica_Polling(data.raw, syscode);
  if ( len == 0 ) 
    return 0;
  
  // low-byte first service code.
  // Suica, Nimoca, etc. 0x090f system 0x0300
  // Edy service 0x170f (0x1317), system 0x00FE // 8280
  // FCF 1a8b
  word servcode = 0x1a8b;
  word scver = nfcreader.felica_RequestService(servcode);
#ifdef DEBUG
  Serial.print("scver = ");
  Serial.println(scver, HEX);
#endif
  if ( scver == 0xffff ) 
    return 0;
  //
  //printf("%04x ver %04x.\n", servcode, scver);
  word blist[] = { 0, 1, 2, 3};
  c = nfcreader.felica_ReadBlocksWithoutEncryption(data.raw, servcode, (byte) 4, blist);
  if ( c == 0 ) {
    //printf("\nfailed reading FCF blocks. \n");
    return 0;
  }
//  printf("\n--- End of FCF reading ---\n\n");
  return 1;
}
#endif

uint8 get_MifareBlock(ISO14443Card & card, const uint8_t * key, char data[64]) {
  uint8 res;
#if defined(USE_PN532)
  nfcreader.targetSet(0x10, card.ID, card.IDLength);
         /* Note !!! Once failed to authentication, card's state will be back to the initial state, 
        So the InListPassivTarget or InAutoPoll should be issued again. */
#endif
#if defined(USE_PN532)
  if ( nfcreader.mifare_AuthenticateBlock(4, key) 
  && nfcreader.getCommandResponse(&res) && res == 0) {
#elif defined (USE_SL030)
  if ( nfcreader.auth_sector(4>>2, key) ) {
#endif
#ifdef DEBUG
    Serial.println("Auth succeeded.");
#endif
#if defined(USE_PN532)
    if ( nfcreader.mifare_ReadBlock(4, data)
      && nfcreader.mifare_ReadBlock(5, data+16)
      && nfcreader.mifare_ReadBlock(6, data+32)
//      && nfcreader.mifare_ReadBlock(7, data+48) 
		)
#elif defined (USE_SL030)
    if ( nfcreader.read_block(4, (uint8*) data)
      && nfcreader.read_block(5, (uint8*) data+16)
      && nfcreader.read_block(6, (uint8*) data+32)
//      && nfcreader.read_block(7, (uint8*) data+48) 
		)
#endif
      return 1;
    Serial.println("Read data blocks failed. ");
  } 
  Serial.println("Authentication failure.");
  return 0;
}


