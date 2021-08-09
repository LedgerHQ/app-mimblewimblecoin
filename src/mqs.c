// Header files
#include <string.h>
#include "base58.h"
#include "common.h"
#include "currency_information.h"
#include "mqs.h"


// Global variables

// MQS data
struct MqsData mqsData;


// Constants

// MQS address private key index
const uint32_t MQS_ADDRESS_PRIVATE_KEY_INDEX = 0;

// MQS shared private key size
const size_t MQS_SHARED_PRIVATE_KEY_SIZE = 32;

// MQS shared private key salt size
const size_t MQS_SHARED_PRIVATE_KEY_SALT_SIZE = 8;

// MQS shared private key number of iterations
const unsigned int MQS_SHARED_PRIVATE_KEY_NUMBER_OF_ITERATIONS = 100;

// Version size
static const size_t VERSION_SIZE = sizeof(uint16_t);

// Salt padding size
static const size_t SALT_PADDING_SIZE = sizeof("\x00\x00\x00\x00") - sizeof((char)'\0');


// Function prototypes

// Get version
uint16_t getVersion(enum NetworkType networkType);


// Supporting function implementation

// Reset MQS data
void resetMqsData(void) {

	// Clear the MQS data
	explicit_bzero(&mqsData, sizeof(mqsData));
}

// Create MQS shared private key
void createMqsSharedPrivateKey(volatile uint8_t *sharedPrivateKey, uint32_t account, const uint8_t *publicKey, uint8_t *salt) {

	// Initialize private key
	volatile cx_ecfp_private_key_t privateKey;

	// Begin try
	BEGIN_TRY {
	
		// Try
		TRY {

			// Uncompress the public key
			uint8_t uncompressedPublicKey[UNCOMPRESSED_PUBLIC_KEY_SIZE];
			memcpy(uncompressedPublicKey, publicKey, COMPRESSED_PUBLIC_KEY_SIZE);
			uncompressSecp256k1PublicKey(uncompressedPublicKey);
			
			// Get private key at the MQS address private key index
			getAddressPrivateKey(&privateKey, account, MQS_ADDRESS_PRIVATE_KEY_INDEX, CX_CURVE_SECP256K1);
			
			// Multiply the uncompressed public key by the private key
			cx_ecfp_scalar_mult(CX_CURVE_SECP256K1, uncompressedPublicKey, UNCOMPRESSED_PUBLIC_KEY_SIZE, (uint8_t *)privateKey.d, privateKey.d_len);
			
			// Check if target is the Nano X
			#ifdef TARGET_NANOX
			
				// Pad the salt
				uint8_t paddedSalt[MQS_SHARED_PRIVATE_KEY_SALT_SIZE + SALT_PADDING_SIZE];
				memcpy(paddedSalt, salt, MQS_SHARED_PRIVATE_KEY_SALT_SIZE);
				explicit_bzero(&paddedSalt[MQS_SHARED_PRIVATE_KEY_SALT_SIZE], SALT_PADDING_SIZE);
				
				// TODO test cx_pbkdf2_sha512 on real hardware to see if the padding is necessary
				
				// Get shared private key from the tweaked uncompressed public key and padded salt
				cx_pbkdf2_sha512(&uncompressedPublicKey[PUBLIC_KEY_PREFIX_SIZE], COMPRESSED_PUBLIC_KEY_SIZE - PUBLIC_KEY_PREFIX_SIZE, paddedSalt, sizeof(paddedSalt), MQS_SHARED_PRIVATE_KEY_NUMBER_OF_ITERATIONS, (uint8_t *)sharedPrivateKey, MQS_SHARED_PRIVATE_KEY_SIZE);
			
			// Otherwise
			#else
			
				// Get shared private key from the tweaked uncompressed public key and salt
				cx_pbkdf2_sha512(&uncompressedPublicKey[PUBLIC_KEY_PREFIX_SIZE], COMPRESSED_PUBLIC_KEY_SIZE - PUBLIC_KEY_PREFIX_SIZE, salt, MQS_SHARED_PRIVATE_KEY_SALT_SIZE, MQS_SHARED_PRIVATE_KEY_NUMBER_OF_ITERATIONS, (uint8_t *)sharedPrivateKey, MQS_SHARED_PRIVATE_KEY_SIZE);
			
			#endif
		}
		
		// Finally
		FINALLY {
		
			// Clear the private key
			explicit_bzero((cx_ecfp_private_key_t *)&privateKey, sizeof(privateKey));
		}
	}
	
	// End try
	END_TRY;
}

// Get public key from MQS address
bool getPublicKeyFromMqsAddress(cx_ecfp_public_key_t *publicKey, const uint8_t *mqsAddress, size_t length, enum NetworkType networkType) {

	// Check if length is invalid
	if(length != MQS_ADDRESS_SIZE) {
	
		// Return false
		return false;
	}
	
	// Get decoded MQS address length
	const size_t decodedMqsAddressLength = getBase58DecodedLengthWithChecksum(mqsAddress, length);
	
	// Check if decoded MQS address length is invalid
	if(decodedMqsAddressLength != VERSION_SIZE + COMPRESSED_PUBLIC_KEY_SIZE + BASE58_CHECKSUM_SIZE) {
	
		// Return false
		return false;
	}
	
	// Check if decoding MQS address failed
	uint8_t decodedMqsAddress[decodedMqsAddressLength];
	if(!base58DecodeWithChecksum(decodedMqsAddress, mqsAddress, length)) {
	
		// Return false
		return false;
	}
	
	// Check if decoded MQS address is invalid
	if(getVersion(networkType) != *(uint16_t *)decodedMqsAddress || !isValidSecp256k1PublicKey(&decodedMqsAddress[VERSION_SIZE], COMPRESSED_PUBLIC_KEY_SIZE)) {
	
		// Return false
		return false;
	}
	
	// Check if getting the public key
	if(publicKey) {
	
		// Uncompress the decoded MQS address to an secp256k1 public key
		uint8_t uncompressedPublicKey[UNCOMPRESSED_PUBLIC_KEY_SIZE];
		memcpy(uncompressedPublicKey, &decodedMqsAddress[VERSION_SIZE], COMPRESSED_PUBLIC_KEY_SIZE);
		uncompressSecp256k1PublicKey(uncompressedPublicKey);
		
		// Initialize the public key with the uncompressed public key
		cx_ecfp_init_public_key(CX_CURVE_SECP256K1, uncompressedPublicKey, sizeof(uncompressedPublicKey), publicKey);
	}

	// Return true
	return true;
}

// Get MQS address from public key
void getMqsAddressFromPublicKey(uint8_t *mqsAddress, const uint8_t *publicKey, enum NetworkType networkType) {

	// Get version
	const uint16_t version = getVersion(networkType);
	
	// Get address data from version and the public key
	uint8_t addressData[sizeof(version) + COMPRESSED_PUBLIC_KEY_SIZE + BASE58_CHECKSUM_SIZE];
	memcpy(addressData, &version, sizeof(version));
	memcpy(&addressData[sizeof(version)], publicKey, COMPRESSED_PUBLIC_KEY_SIZE);
	
	// Encode the address data to get the MQS address
	base58EncodeWithChecksum(mqsAddress, addressData, sizeof(addressData));
}

// Get Mqs address
void getMqsAddress(uint8_t *mqsAddress, uint32_t account, enum NetworkType networkType) {

	// Initialize address private key
	volatile cx_ecfp_private_key_t addressPrivateKey;
	
	// Initialize address public key
	volatile uint8_t addressPublicKey[COMPRESSED_PUBLIC_KEY_SIZE];
	
	// Begin try
	BEGIN_TRY {
	
		// Try
		TRY {
		
			// Get address private key at the MQS address private key index
			getAddressPrivateKey(&addressPrivateKey, account, MQS_ADDRESS_PRIVATE_KEY_INDEX, CX_CURVE_SECP256K1);
			
			// Get address public key from the address private key
			getPublicKeyFromPrivateKey((uint8_t *)addressPublicKey, (cx_ecfp_private_key_t *)&addressPrivateKey);
		}
		
		// Finally
		FINALLY {
		
			// Clear the address private key
			explicit_bzero((cx_ecfp_private_key_t *)&addressPrivateKey, sizeof(addressPrivateKey));
		}
	}
	
	// End try
	END_TRY;
	
	// Get MQS address from the address public key
	getMqsAddressFromPublicKey(mqsAddress, (uint8_t *)addressPublicKey, networkType);
}

// Get version
uint16_t getVersion(enum NetworkType networkType) {

	// Initialize version
	uint8_t version[VERSION_SIZE];

	// Check currency information ID
	switch(currencyInformation.id) {
	
		// MimbleWimble Coin ID
		case MIMBLEWIMBLE_COIN_ID:
		
			// Check network type
			switch(networkType) {
			
				// Mainnet network type
				case MAINNET_NETWORK_TYPE:
				
					// Set version
					version[0] = 1;
					version[1] = 69;
				
					// Break
					break;
				
				// Testnet network type
				case TESTNET_NETWORK_TYPE:
				
					// Set version
					version[0] = 1;
					version[1] = 121;
				
					// Break
					break;
				
				// Default
				default:
				
					// Throw invalid parameters error
					THROW(INVALID_PARAMETERS_ERROR);
			}
			
		// Grin ID
		case GRIN_ID:
		
			// Check network type
			switch(networkType) {
			
				// Mainnet network type
				case MAINNET_NETWORK_TYPE:
				
					// Set version
					version[0] = 1;
					version[1] = 11;
				
					// Break
					break;
				
				// Testnet network type
				case TESTNET_NETWORK_TYPE:
				
					// Set version
					version[0] = 1;
					version[1] = 120;
				
					// Break
					break;
				
				// Default
				default:
				
					// Throw invalid parameters error
					THROW(INVALID_PARAMETERS_ERROR);
			}
		
		// Default
		default:
		
			// Throw invalid parameters error
			THROW(INVALID_PARAMETERS_ERROR);
	}
		
	// Return version
	return *(uint16_t *)version;
}
