#!/usr/bin/perl -w

use strict;
use OBEXFTP;

my $cli = new OBEXFTP::client($OBEXFTP::BLUETOOTH);

my $ret = $cli->connect("00:11:22:33:44:55", 6);
print "$ret\n";

$ret = $cli->list("/");
print "$ret\n";
sleep 2;

$ret = $cli->list("/Data/");
print "$ret\n";
sleep 2;

$ret = $cli->list("/Data/Misc/");
print "$ret\n";
sleep 2;

$ret = $cli->get("/Data/Misc/README.txt");
print "$ret\n";

$ret = $cli->disconnect();
print "$ret\n";

$cli->DESTROY();

