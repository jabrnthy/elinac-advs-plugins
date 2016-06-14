#!/bin/perl -w

# J.Richards 080922
# Create a definitive makestamp for a binary
# $Revision: 1.1.1.1 $
# $Date: 2014/02/04 02:05:04 $
#************************************************************************
use strict;

# code start

##################################################
my $filename = "signature.h";
if (open(INP, $filename)) {
	close INP;
	`rm signature.h`;
}
open(OUT, ">$filename");

my $prod = "NOTDEFINED";
my $c_compiler = 'gcc';
my $libs;

#
#   Translate commandline args into a hash
#
my %args;
foreach my $arg ( @ARGV ){
    if( $arg =~ m/(.+)\s*=\s*(.+$)/ ){
        $args{ $1 } = $2;
    }
}

#
#  Override default values from commandline
#
if( defined( $args{ PRODNAME } ) ){
    $prod = $args{ PRODNAME };
}

if( defined( $args{ COMPILER } ) ){
    $c_compiler = $args{ COMPILER };
}

if( defined( $args{ LIBS } ) ){
    $libs = $args{ LIBS };
}


my $pwd = `pwd`;
chomp $pwd;
my $line ="static char *RELEASE_DIR = \"\\n RELEASE_DIR_$prod $pwd \\n\"\;";
print OUT "$line\n";

my $date = `date`;
chomp $date;
print "$date\n";
$line ="static char *RELEASE_DATE = \"\\n RELEASE_DATE_$prod $date \\n\"\;";
print OUT "$line\n";

my $kernel = `uname -a `;
chomp $kernel;
my @parts = split /#/, $kernel;
$kernel= $parts[0];
print "$kernel\n";
$line ="static char *RELEASE_KERNEL = \"\\n RELEASE_KERNEL_$prod $kernel \\n\"\;";
print OUT "$line\n";

my $cc = `$c_compiler --version`;
chomp $cc;
@parts = split /\n/, $cc;
$cc = $parts[0];
print "$cc\n";
$line ="static char *RELEASE_COMPILER = \"\\n RELEASE_COMPILER_$prod $cc \\n\"\;";
print OUT "$line\n";

my $user = $ENV{USER};
print "$user\n";
$line ="static char *RELEASE_BUILDER = \"\\n RELEASE_BUILDER_$prod $user \\n\"\;";
print OUT "$line\n";

my $epics = $ENV{EPICS_RELEASE};
print "$epics\n";
$line ="static char *RELEASE_EPICS = \"\\n RELEASE_EPICS_$prod $epics \\n\"\;";
print OUT "$line\n";

$line ="void RELEASEShow_$prod";
$line = $line."Support() {";
print OUT "$line\n";
print OUT "	printf (\"\%s\",RELEASE_DIR); \n";
print OUT "	printf (\"\%s\",RELEASE_DATE); \n";
print OUT "	printf (\"\%s\",RELEASE_KERNEL); \n";
print OUT "	printf (\"\%s\",RELEASE_COMPILER); \n";
print OUT "	printf (\"\%s\",RELEASE_BUILDER); \n";
print OUT "	printf (\"\%s\",RELEASE_EPICS); \n";
print OUT "}\n";
close OUT;

