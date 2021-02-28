# Colophony Loyalty BPF program

#### __Solana on-chain program__

## Accounts

- __Program__ -- Loyalty's program of The Colophony on Solana's blockchain. It is owner of accounts of `clients`.
- __Retail network__
	- _Payer_ -- An account which pay a fee running an on-chain `program`. This account registered in `program`.
	- _Blocker_ -- A special account which can blocking an account of `client`.
- __Client__ -- An account produced by the Colophony loyalty program. May be produced by an account of `Payer` of a Retail network only.
- __Client payer__ -- A Solana's account for pay a fee transaction between two `clients`.

## Commands

- __EMIT__ -- points emission by the account of `retail`
- __SPEND__ -- withdrawing points from an account of `client` 
- __TRANSFER__ -- transfer of points between two accounts of `clients`
- __BLOCK__ -- blocking an account of `client`

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