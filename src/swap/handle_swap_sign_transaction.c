#include "globals.h"
#include "os.h"
#include "os_io_seproxyhal.h"
#include "swap_lib_calls.h"
#include "swap_utils.h"
#include "ux.h"

#ifdef HAVE_NBGL
#include "nbgl_use_case.h"
#endif

// Save the BSS address where we will write the return value when finished
static uint8_t *G_swap_sign_return_value_address;

bool copy_transaction_parameters(
    const create_transaction_parameters_t *params) {
  // first copy parameters to stack, and then to global data.
  // We need this "trick" as the input data position can overlap with other
  // apps' globals
  swap_values_t stack_data;
  memset(&stack_data, 0, sizeof(stack_data));

  if (strlen(params->destination_address) >= sizeof(stack_data.destination)) {
    return false;
  }
  strncpy(stack_data.destination, params->destination_address,
          sizeof(stack_data.destination));

  if (!swap_str_to_u64(params->amount, params->amount_length,
                       &stack_data.amount)) {
    return false;
  }

  if (!swap_str_to_u64(params->fee_amount, params->fee_amount_length,
                       &stack_data.fees)) {
    return false;
  }

  // Full reset the global variables
  os_explicit_zero_BSS_segment();
  // Keep the address at which we'll reply the signing status
  G_swap_sign_return_value_address = &params->result;
  // Commit the values read from exchange to the clean global space
  memcpy(&swap_values, &stack_data, sizeof(stack_data));

  return true;
}

void handle_swap_sign_transaction(void) {
  init_globals();
  called_from_swap = true;
  UX_INIT();
#ifdef HAVE_NBGL
  nbgl_useCaseSpinner("Signing");
#endif // HAVE_BAGL
  io_seproxyhal_init();
  USB_power(0);
  USB_power(1);
  PRINTF("USB power ON/OFF\n");
#ifdef HAVE_BLE
  // grab the current plane mode setting
  G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
  BLE_power(0, NULL);
  BLE_power(1, NULL);
#endif // HAVE_BLE
  app_main();
}

void __attribute__((noreturn))
finalize_exchange_sign_transaction(bool is_success) {
  *G_swap_sign_return_value_address = is_success;
  os_lib_end();
}
