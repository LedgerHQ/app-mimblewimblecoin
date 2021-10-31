// Header files
#include <string.h>
#include "chacha20_poly1305.h"
#include "common.h"
#include "continue_decrypting_mqs_data.h"
#include "crypto.h"
#include "currency_information.h"
#include "mqs.h"


// Supporting function implementation

// Process continue decrypting MQS data request
void processContinueDecryptingMqsDataRequest(unsigned short *responseLength, __attribute__((unused)) unsigned char *responseFlags) {

	// Check currency doesn't allow MQS addresses
	if(!currencyInformation.mqsAddressPaymentProofAllowed) {
	
		// Throw unknown instruction error
		THROW(UNKNOWN_INSTRUCTION_ERROR);
	}
	
	// Get request's first parameter
	const uint8_t firstParameter = G_io_apdu_buffer[APDU_OFF_P1];
	
	// Get request's second parameter
	const uint8_t secondParameter = G_io_apdu_buffer[APDU_OFF_P2];
	
	// Get request's data length
	const size_t dataLength = G_io_apdu_buffer[APDU_OFF_LC];
	
	// Get request's data
	const uint8_t *data = &G_io_apdu_buffer[APDU_OFF_DATA];

	// Check if parameters or data are invalid
	if(firstParameter || secondParameter || !dataLength || dataLength > CHACHA20_BLOCK_SIZE) {
	
		// Throw invalid parameters error
		THROW(INVALID_PARAMETERS_ERROR);
	}
	
	// Check if MQS data decrypting state isn't ready or active
	if(mqsData.decryptingState != READY_MQS_DATA_STATE && mqsData.decryptingState != ACTIVE_MQS_DATA_STATE) {
	
		// Throw invalid state error
		THROW(INVALID_STATE_ERROR);
	}
	
	// Initialize decrypted data
	volatile uint8_t decryptedData[dataLength];
	
	// Initialize encrypted data
	volatile uint8_t encryptedData[getEncryptedDataLength(sizeof(decryptedData))];
	
	// Begin try
	BEGIN_TRY {
	
		// Try
		TRY {
	
			// Decrypt ChaCha20 Poly1305 data
			decryptChaCha20Poly1305Data(&mqsData.chaCha20Poly1305State, (uint8_t *)decryptedData, data, dataLength);
			
			// Encrypt the decrypted data
			encryptData(encryptedData, (uint8_t *)decryptedData, sizeof(decryptedData), mqsData.sessionKey, sizeof(mqsData.sessionKey));
		}
		
		// Finally
		FINALLY {
		
			// Clear the decrypted data
			explicit_bzero((uint8_t *)decryptedData, sizeof(decryptedData));
		}
	}
	
	// End try
	END_TRY;
	
	// Check if response with the encrypted data will overflow
	if(willResponseOverflow(*responseLength, sizeof(encryptedData))) {
	
		// Throw length error
		THROW(ERR_APD_LEN);
	}
	
	// Append encrypted data to response
	memcpy(&G_io_apdu_buffer[*responseLength], (uint8_t *)encryptedData, sizeof(encryptedData));
	
	*responseLength += sizeof(encryptedData);
	
	// Check if at the last data 
	if(dataLength < CHACHA20_BLOCK_SIZE) {
	
		// Set that MQS data decrypting state is complete
		mqsData.decryptingState = COMPLETE_MQS_DATA_STATE;
	}
	
	// Otherwise
	else {
	
		// Set that MQS data decrypting state is active
		mqsData.decryptingState = ACTIVE_MQS_DATA_STATE;
	}
	
	// Throw success
	THROW(SWO_SUCCESS);
}
