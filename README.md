# Colophony BPF program

#### __Solana on-chain program__

## Terms

- Retail
- Blocker
- Program -- owner of accounts of `clients`
- Clients -- account on the Solana blockchan


## Commands

- **EMIT** -- points emission by the account of `retail`
- **SPEND** -- withdrawing points from an account of `client` 
- **TRANSFER** -- transfer of points between two accounts of `clients`
- **BLOCK** -- blocking an account of `client`

## Rules

1. Only the `retail` can provide emission and withdrawing points transactions.
   `EMIT` and `SPEND` commands are possible only with the `retail` signature.
   (Attempts to signature such transaction by another account will result an error.)
2. Only the `blocker` can provide blocking account of `client`.
   `LOCK` command is possible only with signature of special account -- `blocker`.
   If account of `client` is marked as locked then `EMIT` and `SPEND` commands for it are unavailable.
3. Anyone can pay for transferring points between `clients`.
   Transfering points are available only between accounts of the program.

## Structure of incoming data

Incoming data (`uint32_t`) include one `command` (two low-order bits) and payload (the others bits).

## Structure of account value

The account data are type of `uint32_t` where one low-order bit is the flag of blocking and others bits are values of the points.