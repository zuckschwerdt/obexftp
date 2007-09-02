#!/usr/bin/perl -w

use strict;
use OBEXFTP;

my $obex = new OBEXFTP::client($OBEXFTP::BLUETOOTH);

my $devs = $obex->discover();
die "No devices found" unless @$devs > 0;
print "Found " . scalar @$devs . " devices\n";
my $dev = $devs->[0]; # or $$devs[0]
my $channel = OBEXFTP::browsebt($dev, 0); # default is ftp
print "Using device $dev on channel $channel\n";
sleep 2;

my $ret = $obex->connect($dev, $channel);
print "$ret\n";

$ret = $obex->list("/");
print "$ret\n";
sleep 2;

$ret = $obex->list("/images");
print "$ret\n";
sleep 2;

$ret = $obex->get("/images/some.jpg");
open OUT, ">downloaded.jpg" or die "Can't write test.out: $@";
binmode(OUT);
print OUT $ret;
close OUT;

$ret = $obex->get("/data/README.txt");
print "$ret\n";

$ret = $obex->disconnect();
print "$ret\n";

$obex->DESTROY();

