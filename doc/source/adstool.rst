#######################
ads, adstool, tcadstool
#######################
Command line tool to access TwinCAT systems via ADS


SYNOPSIS
========
| adstool <target[:port]> [OPTIONS...] <command> [CMD_OPTIONS...] [<command_parameter>...]


DESCRIPTION
===========
`adstool` is a command line tool to access TwinCAT via ADS. Two versions of the
tool exist. `adstool` is based on the open source AdsLib https://github.com/beckhoff/ads
and referenced as "standalone mode", which means it works without a TwinCAT
router on the host system. `tcadstool` is only available on systems with a
local TwinCAT router e.g. TC/BSD or Windows.

The <target> can be AmsNetId, hostname or ip address of the target TwinCAT
system. <port> is optional, its default depends on the <command>.

OPTIONS
=======

--gw=<host>
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Hostname or IP address of your AmsNetId target (mandatory in standalone mode)

--localams=<netid>
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Specify your own AmsNetId (by default derived from local IP + ".1.1")


COMMANDS
===========

addroute --addr=<hostname> --netid=<netid> --password=<password> [--username=Administrator] [--routename=<hostname>]
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Add an ADS route to a remote TwinCAT system. `--username` and `--routename` are
optional. By default "Administrator" user is selected and the value of `--addr`
parameter is used as the name of the route.

file read <path>
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Dump content of the file from <path> to stdout

file delete <path>
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Delete a file from <path>.

file write [--append] <path>
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Read data from stdin write to the file at <path>. File content will be
overwritten unless '--append' is specified.

license platformid
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Read platform id from device.

license systemid
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Read system id from device.

netid
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Read the AmsNetId from a remote TwinCAT router

pciscan <pci_id>
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Show PCI devices with <pci_id>

raw [--read=<number_of_bytes>] <IndexGroup> <IndexOffset>
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
This command gives low level access to:
	- AdsSyncReadReqEx2()
	- AdsSyncReadWriteReqEx2()
	- AdsSyncWriteReqEx()

Read/write binary data at every offset with every length. Data
to write is provided through stdin. Length of the data to write
is determined through the number of bytes provided. If --read
is not provided, the underlying method used will be pure write
request (AdsSyncWriteReqEx()). If no data is provided on stdin,
--read is mandatory and a pure read request (AdsSyncReadReqEx2())
is send. If both, data through stdin and --read, are available,
a readwrite request will be send (AdsSyncReadWriteReqEx2()).

rtime read-latency
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Read latency information from rtime and print it to stdout.

rtime reset-latency
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Read and reset latency information from rtime and print it to stdout.

state [<value>]
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Read or write the ADS state of the device at AmsPort (default 10000).
ADS states are documented here:
https://infosys.beckhoff.com/index.php?content=../content/1031/tcadswcf/html/tcadswcf.tcadsservice.enumerations.adsstate.html

var [--type=<DATATYPE>] <variable name> [<value>]
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
This command gives low level access to:
Reads/Write from/to a given PLC variable.
If value is not set, a read operation will be executed. Otherwise 'value' will
be written to the variable.

On read, the content of a given PLC variable is written to stdout. Format of the
output depends on DATATYPE.

On write, <value> is written to the given PLC variable in an appropriate manner for
that datatype. For strings, <value> will be written as-is. For integers
value will be interpreted as decimal unless it starts with "0x". In that
case it will be interpreted as hex.

DATATYPE:
	BOOL -> default output as decimal
	BYTE -> default output as decimal
	WORD -> default output as decimal
	DWORD -> default output as decimal
	LWORD -> default output as decimal
	STRING -> default output as raw bytes

EXAMPLES
========

Use Administrator account to add a route with the same name as destinations address

.. code-block:: shell

	$ adstool 192.168.0.231 addroute --addr=192.168.0.1 --netid=192.168.0.1.1.1 --password=1

Use 'guest' account to add a route with a selfdefined name

.. code-block:: shell

	$ adstool 192.168.0.231 addroute --addr=192.168.0.1 --netid=192.168.0.1.1.1 --password=1 --username=guest --routename=Testroute

Make a local backup of explorer.exe:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 file read 'C:\Windows\explorer.exe' > ./explorer.exe

Show content of a text file:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 file read 'C:\Temp\hello world.txt'
	Hello World!


Delete a file over ADS and check if it still exists

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 file delete 'C:\Temp\hello world.txt'
	$ adstool 5.24.37.144.1.1 file read 'C:\Temp\hello world.txt'
	$ echo \$?
	1804

Write text directly into a file:

.. code-block:: shell

	$ printf 'Hello World!' | adstool 5.24.37.144.1.1 file write 'C:\Temp\hello world.txt'

Copy local file to remote:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 file write 'C:\Windows\explorer.exe' < ./explorer.exe

Read platformid from device

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 license platformid
	50

Read systemid from device

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 license systemid
	95EEFDE0-0392-1452-275F-1BF9ACCB924E

Read the AmsNetId from a remote TwinCAT router

.. code-block:: shell

	$ adstool 192.168.0.231 netid

List PCI CCAT devices:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 pciscan 0x15EC5000
	PCI devices found: 2
	3:0 @ 0x4028629004
	7:0 @ 0x4026531852

Read 10 bytes from TC3 PLC index group 0x4040 offset 0x1 into a file:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1:851 raw --read=10 "0x4040" "0x1" > read.bin

Write data from file to TC3 PLC index group 0x4040 offset 0x1:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 raw "0x4040" "0x1" < read.bin

Write data from write.bin to TC3 PLC index group 0xF003 offset 0x0 and read result into read.bin:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 raw --read=4 "0xF003" "0x0" < write.bin > read.bin

Read maximum rtime latency

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 rtime read-latency
	6

Read maximum rtime latency and reset:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 rtime reset-latency
	6
	$ adstool 5.24.37.144.1.1 rtime read-latency
	1

Check if TwinCAT is in RUN:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 state
	5

Set TwinCAT to CONFIG mode:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 state 16

Read number as decimal:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 var --type=DWORD "MAIN.nNum1"
	10

Read string:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 var --type=STRING "MAIN.sString1"
	Hello World!

Write a number:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 var --type=DWORD "MAIN.nNum1" "100"

Write a hexvalue:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 var --type=DWORD "MAIN.nNum1" "0x64"

Write string:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 var --type=STRING "MAIN.sString1" "Hello World!"
	$ adstool 5.24.37.144.1.1 var --type=STRING "MAIN.sString1"
	Hello World!

Use quotes to write special characters:

.. code-block:: shell

	$ adstool 5.24.37.144.1.1 var "MAIN.sString1" "STRING" "\"Hello World\""
	$ adstool 5.24.37.144.1.1 var "MAIN.sString1" "STRING"
	"Hello World!"
