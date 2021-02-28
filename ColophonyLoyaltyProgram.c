/**
 * @brief Colophony BPF program
 */
#include <solana_sdk.h>

/** 
 * Custom errors 
*/
#define TO_BUILTIN_CUSTOM(error) ((uint64_t)(error))
/** Permission to perform an operation was denied */
#define ERROR_PERMISSION_DENIED         TO_BUILTIN_CUSTOM(1)
/** Client's account is frozen */
#define ERROR_ACCOUNT_FROZEN            TO_BUILTIN_CUSTOM(2)
/** Client insufficient funds */
#define ERROR_CLIENT_INSUFFICIENT_FUNDS TO_BUILTIN_CUSTOM(3)
/** Client's account storage capacity limit reached */
#define ERROR_STORAGE_CAPACITY_LIMIT    TO_BUILTIN_CUSTOM(4)
/** Cliens, Retail or Blocker not signed a transaction */
#define ERROR_TRANSACTION_NOT_SIGNED    TO_BUILTIN_CUSTOM(5)
/** Cliens's account not writable */
#define ERROR_ACCOUNT_NOT_WRITABLE      TO_BUILTIN_CUSTOM(6)

// Max number of points of client
#define MAX_NUM_OF_POINTS (2147483647U)

// Total number of account needed for work the Colophony BPF program
#define TOTAL_ACCOUNTS 0x2

// Commands
#define EMIT     0x0 // The retail emit points to a client
#define SPEND    0x1 // The retail spend points to a client 
#define FREEZ    0x2 // The retail freeze an account of client
#define TRANSFER 0x3 // A client transfer their points to another client

/**
 * Fields of data an account of client
 */
typedef union
{
  struct 
  {
    uint8_t   isFrozen  : 1;
    uint32_t  value     : 31;
  };
  uint32_t    raw;
} LoyaltyAccountData;

/**
 * Fields of instruction data that input in the program
 */
typedef union
{
  struct
  {
    uint8_t   cmd      : 2;
    uint32_t  payload  : 30;
  };
  uint32_t    raw;
} LoyaltyInstructionData;

/**
 * Structure of the Loyalty account client
 */
typedef struct
{
  SolAccountInfo      * ka;   /** Keyed Account */
  LoyaltyAccountData  * data; /** Fields client's data */
} LoyaltyAccountInfo;

static const SolPubkey RETAIL_PUBLICK_KEY = (SolPubkey){.x = {177,  69, 133, 122, 136, 131, 191,  19,
                                                       135, 158, 203,  39, 147, 209,  30,  85,
                                                       254, 212, 241, 252, 158,  83, 120, 170,
                                                        56,  94, 116, 125,  88, 145,  54,  28}};

static const SolPubkey BLOCKER_PUBLICK_KEY = (SolPubkey){.x = {27,   2, 220,  22, 131, 78, 222,  47,
                                                        225, 107, 102, 236,  43, 15, 171, 140,
                                                        165, 164,  16, 198,  44, 73,  64,  31,
                                                         76, 223, 199, 167,  65, 11, 243, 246}};

static uint64_t checkClient(LoyaltyAccountInfo *const, const LoyaltyInstructionData *const, const SolPubkey *const);
static uint64_t emit(LoyaltyAccountInfo *const, const LoyaltyInstructionData *const);
static uint64_t spend(LoyaltyAccountInfo *const, const LoyaltyInstructionData *const);
static uint64_t freez(LoyaltyAccountInfo *const, const LoyaltyInstructionData *const);
static uint64_t transfer(LoyaltyAccountInfo *const, const LoyaltyInstructionData *const);

typedef uint64_t (*CommandsArray)(LoyaltyAccountInfo *const, const LoyaltyInstructionData *const);
static CommandsArray operations[4] = {&emit, &spend, &freez, &transfer};

/** */
uint64_t ColophonyLoyalty(SolParameters *params)
{
  if (params->ka_num < TOTAL_ACCOUNTS)
	{
    sol_log("#ColophonyLoyalty::Accounts not included in the instruction");
    return ERROR_NOT_ENOUGH_ACCOUNT_KEYS;
  }

  // Interpretation incoming instruction data to the LoyaltyInstructionData union
  const LoyaltyInstructionData *instructionData = (LoyaltyInstructionData *)params->data;
  sol_log("#ColophonyLoyalty::CMD, PAYLOAD"); sol_log_64(instructionData->cmd, instructionData->payload, 0, 0, 0);

  // Get accounts
  LoyaltyAccountInfo *const accounts = (LoyaltyAccountInfo *)sol_calloc(TOTAL_ACCOUNTS, sizeof(LoyaltyAccountInfo));
  accounts[0].ka = &params->ka[0];
  accounts[1].ka = &params->ka[1];
  
  uint64_t errno = 0;
  sol_log("#ColophonyLoyalty::Check account 1");
  if ((errno = checkClient(&accounts[0], instructionData, params->program_id)))
  {
    return errno;
  }
  
  sol_log("#ColophonyLoyalty::Check account 2");
  if (instructionData->cmd == TRANSFER &&
     (errno = checkClient(&accounts[1], instructionData, params->program_id)))
  {
    return errno;
  }

  return operations[instructionData->cmd](accounts, instructionData);
}

/**
 * 
 * @param account Pointer to a LoyaltyAccountInfo structure
 * @param instructionData Pointer to a LoyaltyInstructionData union
 * @param programPublicKey Pointer to a SolPubkey structure
 * @return Error number if unsuccessful, else 0
 */
static uint64_t checkClient(      LoyaltyAccountInfo      *const account,
                            const LoyaltyInstructionData  *const instructionData,
                            const SolPubkey               *const programPublicKey)
{
  const SolAccountInfo * const _account = account->ka;
  sol_log_pubkey(_account->key);
  // Account must be owned by the program in order to modify its data
  if (!SolPubkey_same(_account->owner, programPublicKey))
  { 
    sol_log("Does not have the correct program id");
    sol_log_pubkey(_account->owner);
    return ERROR_INCORRECT_PROGRAM_ID;
  }

  // The data must be large enough to hold an uint32_t value
  if (_account->data_len < sizeof(uint32_t))
  {
    sol_log("Data length too small to hold uint32_t value");
    return ERROR_INVALID_ACCOUNT_DATA;
  }

  // Account of client must be writable
  if (!_account->is_writable)
  {
    sol_log("The client's account not writeble");
    return ERROR_ACCOUNT_NOT_WRITABLE;
  }

  account->data = (LoyaltyAccountData *)_account->data;
  if (account->data->isFrozen && instructionData->cmd != FREEZ)
  {
    sol_log("The client's account is frozen");
    return ERROR_ACCOUNT_FROZEN;
  }

  return SUCCESS;
}

/**
 * Points emission.
 * It can possible only with signature of special account -- Retail.
 * 
 * @param accounts  Pointer to an array of LoyaltyAccountInfo
 * @param instructionData Pointer to a LoyaltyInstructionData union
 * @return Error number if unsuccessful, else 0
 */ 
static uint64_t emit(     LoyaltyAccountInfo *const accounts, 
                    const LoyaltyInstructionData *const instructionData)
{
  sol_log("#ColophonyLoyalty::EMIT");
  const LoyaltyAccountInfo * client = &accounts[0];
  const SolAccountInfo *const retail = accounts[1].ka;

  /** Checking the right to perform an operation */
  if (!SolPubkey_same(retail->key, &RETAIL_PUBLICK_KEY))
  {
    sol_log("Permission denied");
    return ERROR_PERMISSION_DENIED;
  }

  if (!retail->is_signer)
  {
    sol_log("The retail not signed the transaction");
    return ERROR_TRANSACTION_NOT_SIGNED;
  }

  if ((MAX_NUM_OF_POINTS - client->data->value) < instructionData->payload)
  {
    sol_log("Storage of client capacity limit reached");
    sol_log("client.value, payload");
    sol_log_64(client->data->value, instructionData->payload, 0, 0, 0);
    return ERROR_STORAGE_CAPACITY_LIMIT;
  }

  client->data->value += instructionData->payload;

  return SUCCESS;
}

/**
 * Withdrawing client's points.
 * It can possible only with signature of special account -- Retail.
 * 
 * @param accounts  Pointer to an array of LoyaltyAccountInfo
 * @param instructionData Pointer to a LoyaltyInstructionData union
 * @return Error number if unsuccessful, else 0
 */
static uint64_t spend(      LoyaltyAccountInfo *const accounts, 
                      const LoyaltyInstructionData *const instructionData)
{
  sol_log("#ColophonyLoyalty::SPEND");
  const LoyaltyAccountInfo * client = &accounts[0];
  const SolAccountInfo *const retail = accounts[1].ka;
  
  /** Checking the right to perform an operation */
  if (!SolPubkey_same(retail->key, &RETAIL_PUBLICK_KEY))
  {
    sol_log("Permission denied");
    return ERROR_PERMISSION_DENIED;
  }

  if (!retail->is_signer)
  {
    sol_log("The retail not signed the transaction");
    return ERROR_TRANSACTION_NOT_SIGNED;
  }

  if (!client->ka->is_signer)
  {
    sol_log("The client not signed the transaction");
    return ERROR_TRANSACTION_NOT_SIGNED;
  }

  if (client->data->value < instructionData->payload)
  {
    sol_log("Client does not have enough funds");
    sol_log("client.value,  payload");
    sol_log_64(client->data->value,  instructionData->payload, 0, 0, 0);
    return ERROR_CLIENT_INSUFFICIENT_FUNDS;
  }
  
  client->data->value -= instructionData->payload;
  
  return SUCCESS;
}

/**
 * Freeze a client's account.
 * It can possible only with signature of special account -- Blocker.
 * 
 * @param accounts  Pointer to an array of LoyaltyAccountInfo
 * @param instructionData Pointer to a LoyaltyInstructionData union
 * @return Error number if unsuccessful, else 0
 */
static uint64_t freez(      LoyaltyAccountInfo *const accounts, 
                      const LoyaltyInstructionData *const instructionData)
{
  sol_log("#ColophonyLoyalty::FREEZ");
  const LoyaltyAccountInfo * client = &accounts[0];
  const SolAccountInfo *const blocker = accounts[1].ka;

  /** Checking the right to perform an operation */
  if (!SolPubkey_same(blocker->key, &BLOCKER_PUBLICK_KEY))
  {
    sol_log("Permission denied");
    return ERROR_PERMISSION_DENIED;
  }

  if (!blocker->is_signer)
  {
    sol_log("The blocker not signed the transaction");
    return ERROR_TRANSACTION_NOT_SIGNED;
  }
  
  client->data->isFrozen = (instructionData->payload & 0x1);

  return SUCCESS;
}

/**
 * Transfer points between two clients.
 * 
 * @param accounts  Pointer to an array of LoyaltyAccountInfo
 * @param instructionData Pointer to a LoyaltyInstructionData union
 * @return Error number if unsuccessful, else 0
 */
static uint64_t transfer(     LoyaltyAccountInfo *const accounts,
                        const LoyaltyInstructionData *const instructionData)
{
  sol_log("#ColophonyLoyalty::TRANSFER");
  const LoyaltyAccountInfo * sender = &accounts[0];
  const LoyaltyAccountInfo * recipient = &accounts[1];

  if (sender->data->value < instructionData->payload)
  {
    sol_log("Sender client does not have enough funds");
    return ERROR_CLIENT_INSUFFICIENT_FUNDS;
  }

  if ((MAX_NUM_OF_POINTS - recipient->data->value) < instructionData->payload)
  {
    sol_log("Storage capacity limit reached");
    return ERROR_STORAGE_CAPACITY_LIMIT;
  }
  
  sender->data->value -= instructionData->payload;
  recipient->data->value += instructionData->payload;

  return SUCCESS;
}

extern uint64_t entrypoint(const uint8_t *input)
{
  sol_log("#UnoLabs::ColophonyLoyalty program entrypoint");

  SolAccountInfo accounts[2];
  SolParameters params = (SolParameters){.ka = accounts};

  if (!sol_deserialize(input, &params, SOL_ARRAY_SIZE(accounts)))
  {
    return ERROR_INVALID_ARGUMENT;
  }

  return ColophonyLoyalty(&params);
}