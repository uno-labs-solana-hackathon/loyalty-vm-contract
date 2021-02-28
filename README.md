# Colophony Loyalty BPF program

#### __Solana on-chain program__

## Accounts

- __Program__ -- A loyalty program on-chain of the Colophony on Solana's blockchain. It is owner of accounts of `clients`.
- __Retail network__
	- _Payer_ -- An account which pay a fee for the `program` that executes on clockchain. This account's public key is embedded in the `program`.
	- _Blocker_ -- A special account for internal security which can block an account of `client`. The `blocker's` account public key is embedded in the `program` too. 
	- _Client_ -- An account produced by the loyalty program of the Colophony. It may be produced only by the account of the `Payer` of the Retail network.
- __Client payer__ -- A Solana's builtin System program account to pay a fee transaction between two `clients`.

## Commands

- __EMIT__ -- points emission by the account of `retail`
- __SPEND__ -- withdrawing points from an account of `client` 
- __TRANSFER__ -- transfer of points between two accounts of `clients`
- __BLOCK__ -- blocking an account of `client`

## Rules

1. Only the `Payer` of _the Retail network_ can provide emission and withdrawing points transactions.
   `EMIT` and `SPEND` commands are possible only with the `Payer` signature.
   (Attempts to signature such transaction by another account will result an error.)
2. Only the `Blocker` of _the Retail network_ can provide blocking account of a `Client`.
   `LOCK` command is possible only with signature of special account -- the `Blocker`.
   If account of a `Client` is marked as locked then `EMIT` and `SPEND` commands for it are unavailable.
3. Anyone can pay for transferring points between `Clients`. For example it may be the `Client payer`.
   Transfering points are available only between accounts of the program.

## Structure of incoming data

Incoming data (`uint32_t`) include one `command` (two low-order bits) and payload (the others bits).

## Structure of account value

The account data are type of `uint32_t` where one low-order bit is the flag of blocking and others bits are values of the points.