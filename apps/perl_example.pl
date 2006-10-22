#!/usr/bin/perl -w

use strict;
use OBEXFTP;

my $cli = new OBEXFTP::client($OBEXFTP::BLUETOOTH);

my $devs = $cli->discover();
die "No devices found" unless @$devs > 0;
print "Found " . scalar @$devs . " devices\n";
my $dev = $devs->[0]; # or $$devs[0]
my $channel = 6;
print "Using device $dev on channel $channel\n";
sleep 2;

my $ret = $cli->connect($dev, $channel);
print "$ret\n";

$ret = $cli->list("/");
print "$ret\n";
sleep 2;

$ret = $cli->list("/images");
print "$ret\n";
sleep 2;

$ret = $cli->get("/images/some.jpg");
open OUT, ">downloaded.jpg" or die "Can't write test.out: $@";
binmode(OUT);
print OUT $ret;
close OUT;

$ret = $cli->get("/data/README.txt");
print "$ret\n";

$ret = $cli->disconnect();
print "$ret\n";

$cli->DESTROY();

