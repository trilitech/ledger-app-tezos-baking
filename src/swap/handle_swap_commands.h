#pragma once

#include "swap_lib_calls.h"

int handle_check_address(const check_address_parameters_t *params);
int handle_get_printable_amount(get_printable_amount_parameters_t *params);
bool copy_transaction_parameters(const create_transaction_parameters_t *params);
void handle_swap_sign_transaction(void);
bool is_safe_to_swap();

void app_main(void);
void library_main(struct libargs_s *args);

void __attribute__((noreturn)) finalize_exchange_sign_transaction(bool is_success);
