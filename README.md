# Rosin Loyalty

#### __Solana on-chain program (smart contract)__

`Points` are the tokenized asset of corporate loyalty rewards.

The rules for awarding points are determined by the loyalty program server, which is part of the corporate CRM system.

The smart contract does not define the rules for the emission/withdrawing of points, but only keeps track of points in the ledger.

## Accounts

- __Program__ -- An executable account of loyalty program on-chain of the Rosin on Solana's blockchain. It is owner of accounts of `clients`.
- __Retail network__
	- _Payer_ -- An account which pay a fee for the `program` that executes on blockchain. This account's public key is embedded in the `program`.
	- _Blocker_ -- A special account for internal security which can freeze an account of `client`. The `blocker's` account public key is embedded in the `program` too. 
	- _Client_ -- An account produced by the loyalty program of the Rosin. It may be produced only by the account of the `Payer` of the Retail network.
- __Client payer__ -- A Solana's builtin System program account to pay a fee transaction between two `clients`.

## Commands

- __EMIT__ -- points emission to an account of `client`
- __SPEND__ -- withdrawing points from an account of `client`
- __TRANSFER__ -- transfer of points between two accounts of `clients`
- __FREEZ__ -- freeze operations with points on the account of `client`

## Rules

1. Only the `payer` of _the Retail network_ can provide emission and withdrawing points transactions.
   `EMIT` and `SPEND` commands are possible only with the `payer` signature.
   (Attempts to signature such transaction by another account will result an error.)
2. Only the `blocker` of _the Retail network_ can provide freezing account of a `client`.
   `FREEZ` command is possible only with signature of special account -- the `blocker`.
   If account of a `client` is marked as frozen then `EMIT` and `SPEND` commands for it are unavailable.
3. Anyone can pay for transferring points between `clients`. For example it may be the `Client payer`.
   Transfering points are available only between accounts of the program.

## Structure of incoming data

Incoming data (`uint32_t`) include one `command` (two low-order bits) and payload (the others bits).

## Structure of account value

The account data are type of `uint32_t` where one low-order bit is the flag of blocking and others bits are values of the points.
