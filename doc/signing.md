# Signing

The baking application is designed for bakers.

The application enables dedicated baking message to be signed.

To avoid bakers having to check and accept each block and each
consensus operation, the ledger will take care of the checks and will
sign without user intervention.

## Messages

- A `Block`.
- A `Consensus operation`:
  - A `Preattestation`: an operation implements a first vote for a
    candidate block with the aim of building a preattestation quorum.
  - An `Attestation`: an operation implements a vote for a candidate
    block for which a preattestation quorum certificate (PQC) has been
    observed.
  - A `DAL_attestation`: an operation to verify whether attesters were
    able to successfully download the shards assigned to them.
- A `Reveal`: an operation reveals the public key of the sending
  manager. Knowing this public key is indeed necessary to check the
  signature of future operations signed by this manager.
- A `Delegation`: an operation allows users to delegate their stake to
  a delegate (a baker), or to register themselves as delegates.

## Checks

The ledger will refuse to sign:
- a message if the key requested to sign the message is not the
  [`authorized-key`](NVRAM.md#authorized-key).
- Relative to the [`HWM`](NVRAM.md#hwm) of the corresponding [`chain-id`](NVRAM.md#chain-id):
  - an `Attestation` if another `Attestation` has already been signed
    by the ledger at the same level and round or higher (a higher
    level or the same level, but a higher round).
  - a `Pre-attestation` if another `Pre-attestation` or `Attestation`
    has already been signed by the ledger at the same level and round
    or higher.
  - a `Block` if another `Block`, a `Pre-attestation` or an
    `Attestation` has already been signed by the ledger at the same
    level and in the same round or higher.
- a manager operation if it contains:
  - operations other than `Reveal` or `Delegation`. A point to note is that you can only set/unset Delegation using baking app. To stake your tez, you need to use tezos-wallet app.
  - operations with their source different from the [`authorized-key`](NVRAM.md#authorized-key).
  - a `Reveal` with its revealed key different from the
    [`authorized-key`](NVRAM.md#authorized-key) (More than one `Reveal` can be signed in a
    manager operation).
  - more than one `Delegation`.

For signing, a user validation will be required only if the message is
a valid manager operation that contains a `Delegation`.
