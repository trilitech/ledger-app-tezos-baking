# NVRAM

To enable verification, the ledger will retain following data in its Non-volatile memory. It will also modify data after each signing operation.

Following data is saved to NVRAM:

## `authorized-key`

The key only key authorized to sign.

It can be set, given a path and a curve, using [`AUTHORIZE_BAKING`](apdu.md#authorize_baking) and [`SETUP`](apdu.md#setup).
A manual user validation will be required.
And its public key will be returned.

It can be unset using [`DEAUTHORIZE`](apdu.md#deauthorize).

Its path can be retrieved using [`QUERY_AUTH_KEY`](apdu.md#query_auth_key) (and
[`QUERY_AUTH_KEY_WITH_CURVE`](apdu.md#query_auth_key_with_curve) that also gives its curve)

## `chain-id`

The main chain id.

If no chain is registered, all chains encounter will be considered as main.

It can be set using [`SETUP`](apdu.md#setup) and retrieved using [`QUERY_ALL_HWM`](apdu.md#query_all_hwm).

## `HWM`

Two High Water Mark representing:
 - the main chain HWM.
 - the test chain HWM.

Each HWM contains informations about the current state of the chain.
It contains the highest level encounter and the highest round encounter for this level.

The both HWM can be set using [`SETUP`](apdu.md#setup) and retrieved using [`QUERY_ALL_HWM`](apdu.md#query_all_hwm).
