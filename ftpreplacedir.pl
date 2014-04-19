#!/usr/bin/perl -w

use POSIX;
use Net::FTP;
use strict;
use diagnostics;

my $VERSION="10-DEC-2006";

sub delrdir($$);
sub copydir($$$);

#
## Usage: ftreplacedir.pl host user pwd localdir remotedir
#
## Note: remotedir will be remotedirnew during the work, then this will be renamed
##       BLANKS ARE NOT SUPPORTED
##       For password, user the character '-' to fet it from ~/.netrc
#

if ($#ARGV != 4) {
    print STDERR "Usage: $0 host user pwd localdir remotedir\n";
    exit(EXIT_FAILURE);
}

my ($host, $user, $pwd, $localdir, $remotedir) = @ARGV;

my $ftp = Net::FTP->new($host, Debug => 0)   or die "Cannot connect to $host : $@";
if ($pwd ne "-") {
    $ftp->login($user,$pwd)                  or die "Cannot login with \"$user\" name ", $ftp->message;
} else {
    $ftp->login($user)                       or die "Cannot login with \"$user\" name ", $ftp->message;
}
$ftp->binary()                               or die "Cannot put to binary mode", $ftp->message;

#
## Delete remotedirnew
#
delrdir($ftp,"${remotedir}new");
#
## Create remotedirnew
#
print "==> MKDIR ${remotedir}new\n";
$ftp->mkdir("${remotedir}new")               or die "Cannot create remote dir ${remotedir}new : $@";
#
## Copy localdir to remotedirnew
#
copydir($ftp,"$localdir", "${remotedir}new");
#
## Delete remotedir (no way to be atomic doing a rename here)
#
delrdir($ftp,"$remotedir");
#
## Rename remotedirnew to remotedirnew
#
print "==> RENAME ${remotedir}new $remotedir\n";
$ftp->rename("${remotedir}new","$remotedir") or die "Cannot rename ${remotedir}new to $remotedir : $@";

$ftp->quit;

exit(EXIT_SUCCESS);

sub delrdir($$) {
    my $ftp = shift;
    my $rdir = shift;
    my $isdir;
    my @list;

    @list = $ftp->dir($rdir);

    if ($#list > 0) {
	#
	## If it is a directory it will have . and ..
	#
	foreach (@list) {
	    #
	    ## We suppose there is NO BLANK IN FILENAMES
	    #
	    my $file = (split(/ /))[-1];
	    if ($file ne "." && $file ne "..") {
		delrdir($ftp, "$rdir/$file");
	    }
	}
	print "==> RMDIR $rdir\n";
	$ftp->rmdir($rdir)
	    or die "Cannot remove remote dir $rdir : $@";
    } elsif ($#list == 0) {
	#
	## If it is a file it be a single entry
	#
	print "==> DEL $rdir\n";
	$ftp->delete($rdir)
	    or die "Cannot remove remote dir $rdir : $@";
    } else {
	print "==> $rdir does not exist\n";
    }
}

sub copydir($$$) {
    my $ftp = shift;
    my $ldir = shift;
    my $rdir = shift;
    my $isldir;
    my $isrdir;
    my @list;

    #
    ## Get local directory listing - This will exit if ldir is not a dir
    #
    opendir(DIR, $ldir) || die "can't opendir $ldir : $!";
    @list = readdir(DIR);
    closedir DIR;

    foreach (@list) {
	if ($_ ne "." && $_ ne "..") {
	    if (-d "$ldir/$_") {
		print "==> MKDIR $rdir/$_\n";
		$ftp->mkdir("$rdir/$_")
		    or die "Cannot create remote dir $rdir/$_ : $@";
		copydir($ftp, "$ldir/$_", "$rdir/$_");
	    } else {
		print "==> PUT $ldir/$_ $rdir/$_\n";
		my $output = $ftp->put("$ldir/$_", "$rdir/$_");
		if ($output ne "$rdir/$_") {
		    die "Cannot put remote file $rdir/$_ : $@";
		}
	    }
	}
    }
}
