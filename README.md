# Tezos Ledger Applications

## Investigation


## Overview

This repository contains Ledger application to support baking on Tezos blockchain:

1. The "Tezos Baking" application (Nano S only) is for baking: signing new blocks and
endorsements. For more information about baking, see
*[Benefits and Risks of Home Baking](https://medium.com/@tezos_91823/benefits-and-risks-of-home-baking-a631c9ca745)*.

It is possible to do all of these things without a hardware wallet, but using a
hardware wallet provides you better security against key theft.


## Hacking

See [CONTRIBUTING.md](CONTRIBUTING.md)


### udev rules (Linux only)

You need to set `udev` rules to set the permissions so that your user account
can access the Ledger device. This requires system administration privileges
on your Linux system.

#### Instructions for most distros (not including NixOS)

LedgerHQ provides a
[script](https://raw.githubusercontent.com/LedgerHQ/udev-rules/master/add_udev_rules.sh)
for this purpose. Download this script, read it, customize it, and run it as root:

```
$ wget https://raw.githubusercontent.com/LedgerHQ/udev-rules/master/add_udev_rules.sh
$ chmod +x add_udev_rules.sh
```

We recommend against running the next command without reviewing the
script and modifying it to match your configuration.

```
$ sudo ./add_udev_rules.sh
```

Subsequently, unplug your ledger hardware wallet, and plug it in again for the changes to take
effect.

#### Instructions for NixOS

For NixOS, you can set the udev rules by adding the following to the NixOS
configuration file typically located at `/etc/nixos/configuration.nix`:

```nix
hardware.ledger.enable = true;
```

Once you have added this, run `sudo nixos-rebuild switch` to activate the
configuration, and unplug your Ledger device and plug it in again for the changes to
take effect.

## Installing the Applications with Ledger Live

The easiest way to obtain and install the Tezos Ledger apps is to download them
from [Ledger Live](https://www.ledger.com/pages/ledger-live). Tezos Baking is available when you enable 'Developer Mode' in Settings.

If you've used Ledger Live for application installation, you can skip the following section.

## Obtaining the Applications without Ledger Live

Download the source code for application from github repository [App-tezos-baking](https://github.com/LedgerHQ/app-tezos).

### Building

```
$ git clone https://github.com/LedgerHQ/app-tezos.git
$ cd app-tezos
```
Then run the following command to enter into docker container provided by Ledger. You will need to have  docker cli installed.
Use the docker container `ledger-app-dev-tools` provided by Ledger to build the app.

```
docker run --rm -ti -v $(pwd):/app ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools:latest
```
Then build the baking app inside the docker container shell as follows :

```
BOLOS_SDK=$NANOS_SDK make
```
To enable debugging and log output, use
```
BOLOS_SDK=$NANOS_SDK make DEBUG=1
```
You can replace `NANOS` with `NANOSP`, `NANOX`, `STAX` for the other devices in BOLOS_SDK environmental variable.

### Testing
To test the application you need to enable python virtual environment and some dependencies. On any operating system, create a python virtual environment and activate it.
```
$ sudo apt-get update && sudo apt-get install -y qemu-user-static tesseract-ocr libtesseract-dev python3-pip libgmp-dev libsodium-dev git
$ python3 -m venv env
$ source env/bin/activate
```
Inside the virtualenv, load the requirements.txt file.
```
(env)$ pip install -r test/python/requirements.txt
```
Now you can run ragger tests for any perticular ledger device. Please make sure you have built the app.elf files for that perticular device first. Then run following command:
```
(env)$ pytest test/python --device nanosp
```
Replace nanosp with any of the following for respective device: nanos, nanosp, nanox , stax.


### Installing the apps onto your Ledger device without Ledger Live

Manually installing the apps requires a command-line tool called LedgerBlue

### Installing BOLOS Python Loader

Install `libusb` and `libudev`, with the relevant headers. On Debian-based
distros, including Ubuntu, the packages with the headers are suffixed with
`-dev`. Other distros will have their own conventions. So, for example, on
Ubuntu, you can do this with:

```
$ sudo apt-get install libusb-1.0.0-dev libudev-dev # Ubuntu example
```
Then, you must enter the `env`. If you do not successfully enter the `env`,
future commands will fail. You can tell you have entered the virtualenv when your prompt is
prefixed with `(ledger)`. Use the following command to enter `env`.

```
$ source env/bin/activate
```

Your terminal session -- and only that terminal session -- will now be in the
virtual env. To have a new terminal session enter the virtualenv, run the above
`source` command only in the same directory in the new terminal session.

### ledgerblue: The Python Module for Ledger Nano S/X

We can now install `ledgerblue`, which is a Python module designed originally for
Ledger Blue, but also is needed for the Ledger Nano S/X.
```
$ python3 -m pip install ledgerblue
```

[Python package](https://pypi.org/project/ledgerblue/).
This will install the Ledger Python packages into the virtualenv; they will be
available only in a shell where the virtualenv has been activated.

If you have to use `sudo` or `pip3` here, that is an indication that you have
not correctly set up `virtualenv`. It will still work in such a situation, but
please research other material on troubleshooting `virtualenv` setup.

### Load the application onto the Ledger device

Next you'll use the installation script to install the application on your Ledger device.

The Ledger device must be in the following state:

  * Plugged into your computer
  * Unlocked (enter your PIN)
  * On the home screen (do not have any application open)
  * Not asleep (you should not see Ledger screensaver across the
    screen)

If you are already in an application or the Ledger device is asleep, your installation process
will fail.

We recommend staying at your computer and keeping an eye on the Ledger device's screen
as you continue. You may want to read the rest of these instructions before you
begin installing, as you will need to confirm and verify a few things during the
process.

Make sure you have built the appropriate device files by following the 'Building' section. We will be using the `app.apdu` and `app.elf` from `build/<device>` directory.  Here `<device>` can take values nanos, nanos2 (for Nanosp), nanox and stax.
```
$ python3 -m ledgerblue.runScript  --scp --fileName build/<device>/bin/app.apdu --elfFile build/<device>/bin/app.elf
```

The first thing that should come up in your terminal is a message that looks
like this:

```
Generated random root public key : <long string of digits and letters>
```

Look at your Ledger device's screen and verify that the digits of that key match the
digits you can see on your terminal. What you see on your Ledger hardware wallet's screen
should be just the beginning and ending few characters of the longer string that
printed in your terminal.

You will need to push confirmation buttons on your Ledger device a few times
during the installation process and re-enter your PIN code near the end of the
process. You should finally see the Tezos logo appear on the screen.

If you see the "Generated random root public key" message and then something
that looks like this:

```
Traceback (most recent call last):
File "/usr/lib/python3.6/runpy.py", line 193, in _run_module_as_main
<...more file names...>
OSError: open failed
```

the most likely cause is that your `udev` rules are not set up correctly, or you
did not unplug your Ledger hardware wallet between setting up the rules and attempting to
install. Please confirm the correctness of your `udev` rules.

To load a new version of the Tezos application onto the Ledger device in the future,
you can run the command again, and it will automatically remove any
previously-loaded version.

### Removing Your App

If you'd like to remove your app, you can do this. In the virtualenv
described in the last sections, run this command:

```
$ python -m ledgerblue.deleteApp --targetId 0x31100004 --appName 'Tezos Baking'
```

Replace the `appName` parameter "Tezos" with whatever application name you used when you loaded the application onto the device.

Then follow the prompts on the Ledger device screen.

### Confirming the Installation Worked

You should now have `Tezos Baking` app installed on the device. The `Tezos Baking` application should display a `0` under screen on the screen ,`Highest Watermark` which is the highest block level baked so far (`0` in case of no blocks).

## Using the Tezos Baking Application (Nano S only)

The Tezos Baking Application supports the following operations:

  1. Get public key
  2. Setup ledger for baking
  3. Reset high watermark
  4. Get high watermark
  5. Sign (blocks and endorsements)

It will only sign block headers and endorsements, as the purpose of the baking
application is that it cannot be co-opted to perform other types of operations (like
transferring XTZ). If a Ledger device is running with the Tezos Baking Application, it
is the expectation of its owner that no transactions will need to be signed with
it. To sign transactions with that Ledger device, you will need to switch it to using
the Tezos Wallet application, or have the Tezos Wallet application installed on
a paired device. Therefore, if you have a larger stake and bake frequently, we
recommend the paired device approach. If, however, you bake infrequently and can
afford to have your baker offline temporarily, then switching to the Tezos
Wallet application on the same Ledger device should suffice.


### Setup Ledger with Tezos client

To connect ledger with Tezos client, you need to download [Tezos](https://www.gitlab.com/tezos/tezos).
You need to have nix installed on your system. Build tezos with following commands:
```
$ git clone https://gitlab.com/tezos/tezos.git
$ cd tezos
$ nix-shell -j auto
$ make
```
This will build the latest version of tezos repo.
Now connect the ledger device to USB port of your computer and run following command:
```
$ ./octez-client list connected ledgger
```
It will given output as follows:
```
## Ledger `masculine-pig-stupendous-dugong`
Found a Tezos Baking 2.4.7 (git-description: "v2.4.7-70-g3195b4d2")
application running on Ledger Nano S Plus at [1-1.4.6:1.0].

To use keys at BIP32 path m/44'/1729'/0'/0' (default Tezos key path), use one
of:
  octez-client import secret key ledger_username "ledger://masculine-pig-stupendous-dugong/ed25519/0h/0h"
  octez-client import secret key ledger_username "ledger://masculine-pig-stupendous-dugong/secp256k1/0h/0h"
  octez-client import secret key ledger_username "ledger://masculine-pig-stupendous-dugong/P-256/0h/0h"
  octez-client import secret key ledger_username "ledger://masculine-pig-stupendous-dugong/bip25519/0h/0h"
```
Here the last four lines give information about the available keys you can use to sign blocks/attestations etc. in baking. The names in front of ledger:// are generated randomly and represent a unique path to key derivations in ledger. Choose one of the keys listed above as follows:
```
$ ./octez-client import secret key ledger_username "ledger://masculine-pig-stupendous-dugong/bip25519/0h/0h"
```
Here we have chosen the last key type bip25519. You can choose any one of the available keys.

You can verify that you have successfully setup ledger with following command:
```
$ ./octez-client list known addresses
```
It will show output as follows:
```
ledger_<...>: tz1N4GQ8gYgMdq6gUsja783KJButHUHn5K7z (ledger sk known)
```
You can use the address ledger_<...> for further commands to setup the baking operations with Ledger.

### Setup Node and baker

It is recommended to practice baking on tezos testnet before you acutally start baking on mainnet  with real money. You can get more information
about baking on testnet at [Baking-setup-Tutorial](https://docs.tezos.com/tutorials/join-dal-baker).
Here we only give information about changes you have to make in above tutorial to bake with Ledger instead of an auto generated key.

In the tutorial skip the command `octez-client gen keys my_baker` and instead use the ledger_<...> in place of my_baker.
Use the following command to store your address in environmental variable `MY_BAKER`
```
$ MY_BAKER="$(./octez-client show address ledger_<...> | head -n 1 | cut -d ' ' -f 2)"
```

### Setup ledger device to bake and endorse

You need to run a specific command to authorize a key for baking. Once a key is
authorized for baking, the user will not have to approve this command again. If
a key is not authorized for baking, signing endorsements and block headers with
that key will be rejected. This authorization data is persisted across runs of
the application, but not across application installations. Only one key can be authorized for baking per Ledger hardware wallet at a
time.

In order to authorize a public key for baking, use the APDU for setting up the ledger device to bake:

    ```
    $ octez-client setup ledger to bake for ledger_<...>
    ```

    This only authorizes the key for baking on the Ledger device, but does
    not inform the blockchain of your intention to bake. This might
    be necessary if you reinstall the app, or if you have a different
    paired Ledger device that you are using to bake for the first time.

### Registering as a Delegate

*Note: The ledger device will not sign this operation unless you have already setup the device to bake using the command in the previous section.*

In order to bake from the Ledger device account you need to register the key as a
delegate. This is formally done by delegating the account to itself. As a
non-originated account, an account directly stored on the Ledger device can only
delegate to itself.

Open the Tezos Baking Application on the device, and then run this:

```
$ octez-client register key ledger_<...> as delegate
```

This command is intended to inform the blockchain itself of your intention to
bake with this key.

### Stake tez to get baking rights
Currently baking app does not support signing transactions. You need to stake certain amount of tez to get baking rights. Install Tezos wallet app on the same ledger device and run following command. No setup is needed as we have already setup the address from which we are deducting the amount.
```
$ octez-client stake <amount> ledger_<...>
```

### Sign

The sign operation is for signing block headers and endorsements.

Block headers must have monotonically increasing levels; that is, each
block must have a higher level than all previous blocks signed with the Ledger device.
This is intended to prevent double baking and double endorsing at the device level, as a security
measure against potential vulnerabilities where the computer might be tricked
into double baking. This feature will hopefully be a redundant precaution, but
it's implemented at the device level because the point of the Ledger hardware wallet is to not
trust the computer. The current High Watermark (HWM) -- the highest level to
have been baked so far -- is displayed on the device's screen, and is also
persisted between runs of the device.

The sign operation will be sent to the hardware wallet by the baking daemon when
configured to bake with a Ledger device key. The Ledger device uses the first byte of the
information to be signed -- the magic number -- to tell whether it is a block
header (which is verified with the High Watermark), an endorsement (which is
not), or some other operation (which it will reject, unless it is a
self-delegation).

With the exception of self-delegations, as long as the key is configured and the
high watermark constraint is followed, there is no user prompting required for
signing. Tezos Baking will only ever sign without prompting or reject an
attempt at signing; this operation is designed to be used unsupervised. As mentioned,
 the only exception to this is self-delegation.

### Security during baking

The Tezos-Baking app needs to be kept open during baking and ledger is unlocked during that time. To prevent screen burn, the baking app goes into blank screen when it starts signing blocks/attestation as baker. But the app remains unlocked. One can not sign any transaction operation using baking app, therefore there is no need of any concern. But to exit the baking app, one needs to enter PIN. This restriction is in place to avoid misuse of physical ledger device when its kept unattended during baking process.

### Reset High Watermark

When updating the version of Tezos Baking you are using or if you are switching baking to
 a new ledger device, we recommend setting the HWM to the current head block level of the blockchain.
This can be accomplished with the reset command. The following command requires an explicit
confirmation from the user:

```
$ octez-client set ledger high watermark for "ledger://<tz...>/" to <HWM>
```

`<HWM>` indicates the new high watermark to reset to. Both the main and test chain HWMs will be
simultaneously changed to this value.

If you would like to know the current high watermark of the ledger device, you can run:

```
$ octez-client get ledger high watermark for "ledger://<tz...>/"
```

While the ledger device's UI displays the HWM of the main chain it is signing on, it will not
display the HWM of a test chain it may be signing on during the 3rd period of the Tezos Amendment Process.
Running this command will return both HWMs as well as the chain ID of the main chain.

## Upgrading

When you want to upgrade to a new version, whether you built it yourself from source
or whether it's a new release of the `app.hex` files, use the same commands as you did
to originally install it. As the keys are generated from the device's seeds and the
derivation paths, you will have the same keys with every version of this Ledger hardware wallet app,
so there is no need to re-import the keys with `octez-client`. You may need to run command `octez-client setup ledger to bake for ...` again as HWM and chain information would be erased after reinstalling the app.

### Special Upgrading Considerations for Bakers

If you've already been baking on an old version of Tezos Baking, the new version will
not remember which key you are baking with nor the High Watermark. You will have to re-run
this command to remind the hardware wallet what key you intend to authorize for baking. As shown, it can
also set the HWM:

```
$ octez-client setup ledger to bake for ledger_<...> --main-hwm <HWM>
```

Alternatively, you can also set the High Watermark to the level of the most recently baked block with a separate command:

```
$ octez-client set ledger high watermark for "ledger://<tz...>/" to <HWM>
```

The latter will require the correct URL for the Ledger device acquired from:

```
$ octez-client list connected ledgers
```

## Troubleshooting

### Display Debug Logs

To debug the application you need to compile the application with `DEBUG=1`

### Importing a Fundraiser Account to a Ledger Device

You currently cannot directly import a fundraiser account to the Ledger device. Instead, you'll first need to import your fundraiser account to a non-hardware wallet address from which you can send the funds to an address on the ledger. You can do so with wallet providers such as [Galleon](https://galleon-wallet.tech/) or [TezBox](https://tezbox.com/).

### Two Ledger Devices at the Same Time

Two Ledger devices with the same seed should not ever be plugged in at the same time. This confuses
`octez-client` and other client programs. Instead, you should plug only one of a set of paired
ledgers at a time. Two Ledger devices of different seeds are fine and are fully supported,
and the computer will automatically determine which one to send information to.

If you have one running the baking app, it is bad for security to also have the wallet app
plugged in simultaneously. Plug the wallet application in as-needed, removing the baking app, at a time
when you are not going to be needed for endorsement or baking. Alternatively, use a different
computer for wallet transactions.

### unexpected seq num

```
$ octez-client list connected ledgers
Fatal error:                                                                                                                                        Header.check: unexpected seq num
```

This means you do not have the Tezos application open on your device.

### No device found

```
$ octez-client list connected ledgers
No device found.
Make sure a Ledger device is connected and in the Tezos Wallet app.
```

In addition to the possibilities listed in the error message, this could also
mean that your udev rules are not set up correctly.

### Unrecognized command

If you see an `Unrecognized command` error, it might be because there is no node for `octez-client`
to connect to. Please ensure that you are running a node. `ps aux | grep tezos-node` should display
the process information for the current node. If it displays nothing, or just displays a `grep`
command, then there is no node running on your machine.

### Error "Unexpected sequence number (expected 0, got 191)" on macOS

If `octez-client` on macOS intermittently fails with an error that looks like

```
client.signer.ledger: APDU level error: Unexpected sequence number (expected 0, got 191)
```

then your installation of `octez-client` was built with an older version of HIDAPI that doesn't work well with macOS (see [#30](https://github.com/obsidiansystems/ledger-app-tezos/issues/30)).

To fix this you need to get the yet-unreleased fixes from the [HIDAPI library](https://github.com/signal11/hidapi) and rebuild `octez-client`.

If you got HIDAPI from Homebrew, you can update to the `master` branch of HIDAPI like this:

```shell
$ brew install hidapi --HEAD
```

Then start a full rebuild of `octez-client` with HIDAPI's `master` branch:

```shell
$ brew unlink hidapi   # remove the current one
$ brew install autoconf automake libtool  # Just keep installing stuff until the following command succeeds:
$ brew install hidapi --HEAD
```

Finally, rebuild `ocaml-hidapi` with Tezos. In the `tezos` repository:

```shell
$ opam reinstall hidapi
$ make all build-test
$ ./octez-client list connected ledgers  # should now work consistently
```

Note that you may still see warnings similar to `Unexpected sequence number (expected 0, got 191)` even after this update. The reason is that there is a separate, more cosmetic, issue in `octez-client` itself which has already been fixed but may not be in your branch yet (see the [merge request](https://gitlab.com/tezos/tezos/merge_requests/600)).

### Command Line Installations: "This app is not genuine"

If you install a Ledger application, such as Tezos Wallet or Tezos Baking, outside of Ledger Live you will see the message "This app is not genuine" followed by an Indentifier when opening the app. This message is generated by the device firmware as a warning to the user whenever an application is installed outside Ledger Live. Ledger signs the applications available in Ledger Live to verify their authenticity, but the same applications available elsewhere, such as from this repo, are not signed by Ledger. As a result, the user is warned that the app is not "genuine", i.e. signed by Ledger. This helps protect users who may have accidentally downloaded an app from a malicious client without knowing it. Note that the application available from this repo's [releases page](https://github.com/obsidiansystems/ledger-app-tezos/releases/tag/v2.2.7) is otherwise no different from the one downloaded from Ledger Live.

## Feedback
To give feedback and report an error, create an issue on github repository [Trillitech-App-Tezos](https://github.com/trillitech/ledger-app-tezos-baking).
