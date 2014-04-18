#!env perl
use strict;
use diagnostics;
use LWP::Simple;
use File::Temp qw/tempdir/;
use File::Slurp;
use File::Basename;
use File::Spec;
use File::Copy;
use Data::Dumper;
use Getopt::Long;
use Log::Log4perl qw/:easy/;
use Log::Any::Adapter;
use Log::Any qw/$log/;
use URI;
use Archive::Tar;
use Cwd;
use File::Find;
use POSIX qw/EXIT_SUCCESS EXIT_FAILURE/;

# ---------
# Constants
# ---------
our $VERSION = '1.0';
our $MAP_FILENAME = 'map.txt';
our $TMP_DIRNAME = tempdir(CLEANUP => 1);

# -------
# Options
# -------
my %opts = (
    libMarpaVersion => 0,
    mapFilename     => $MAP_FILENAME,
    logLevel        => 'WARN',
    debian          => 0,
);
my %cmdOpts = (
    'version=i'         => sub { $opts{libMarpaVersion} = $_[1] },
    'mapFilename=s'     => sub { $opts{mapFilename} = $_[1] },
    'debian!'           => sub { $opts{debian} = $_[1] },
    'help!'             => sub { help(\%opts) },
    'verbose!'          => sub { $opts{logLevel} = $_[1] ? 'DEBUG' : 'WARN' },
    );
GetOptions (%cmdOpts) || die "Error in command line arguments";

# ------------
# Init logging
# ------------
initLog(\%opts);

# ----------------------------------------------
# Load mapping libmarpa version <=> CPAN tarball
# ----------------------------------------------
my %map = ();
loadMap($MAP_FILENAME, \%map);

# ----------------------
# Select libMarpaVersion
# ----------------------
if (! $opts{libMarpaVersion}) {
    $opts{libMarpaVersion} = (sort {$b <=> $a} keys %map)[0];
}

# ------------
# Load tarball
# ------------
my $tarball = getCPANTarball($opts{libMarpaVersion}, \%map, $TMP_DIRNAME);

# ---------------
# Extract tarball
# ---------------
my %dirs = (
    libmarpa_dist => undef,
    libmarpa_doc_dist => undef
    );
extractCPANTarball($tarball, $TMP_DIRNAME, \%dirs);

# -------------------
# Process directories
# -------------------
my %newDirs = (
    libmarpa_dist => 'libmarpa',
    libmarpa_doc_dist => 'libmarpa-doc'
    );
processDirs(\%dirs, \%newDirs, \%opts);

exit(EXIT_SUCCESS);

# #############################################################################
# Init logging
# #############################################################################
sub initLog {
    my ($optsp) = @_;

    my $log4perlConf = <<LOG4PERL_CONF;
log4perl.rootLogger              = $optsp->{logLevel}, Screen
log4perl.appender.Screen         = Log::Log4perl::Appender::Screen
log4perl.appender.Screen.stderr  = 0
log4perl.appender.Screen.layout  = PatternLayout
log4perl.appender.Screen.layout.ConversionPattern = %d %-5p %6P %m{chomp}%n
LOG4PERL_CONF
    Log::Log4perl::init(\$log4perlConf);
    Log::Any::Adapter->set('Log4perl');

    $log->debugf('Logging level initialized to %s', $optsp->{logLevel});

    return;
}

# #############################################################################
# Load Map
# #############################################################################
sub loadMap {
    my ($mapFilename, $mapHashp) = @_;

    $log->debugf('Loading %s', $mapFilename);

    my @mapLines = read_file($mapFilename);
    foreach (@mapLines) {
	next if (! /^\s*([0-9.]+)\s+([^\s]+)/);
	$mapHashp->{$1} = $2;
    }

    $log->debugf('Version/URL mapping is %s', $mapHashp);

    return;
}

# #############################################################################
# Help
# #############################################################################
sub help {
    my ($optsp) = @_;

    print <<HELP;
Usage: $^X $0 [options]

where options are all optional and can be:

--map=mapFilename             Mapping libmarpa version <=> marpa CPAN tarball
                              Default value: $optsp->{mapFilename}

--version=libMarpaVersio n    libmarpa version to build.
                              Default value: highest numeric loaded from mapFilename

--verbose                     Verbose mode.

--debian                      Debianize.

--help                        This help.
HELP
    exit(EXIT_SUCCESS);
}

# #############################################################################
# Get CPAN Tarball
# #############################################################################
sub getCPANTarball {
    my ($libMarpaVersion, $mapp, $tmpdir) = @_;

    my $uri = URI->new($mapp->{$libMarpaVersion});
    my $path = $uri->path;
    my $base = basename($path);
    my $dst = File::Spec->catdir($tmpdir, $base);

    $log->debugf('Saving %s to %s', "$uri", "$dst");

    getstore($uri, $dst) || die "Cannot save $uri to $dst\n";

    $log->debugf('Size of %s: %d bytes', $dst, -s $dst);

    return $dst;
}

# #############################################################################
# Extract CPAN Tarball
# #############################################################################
sub extractCPANTarball {
    my ($tarball, $tmpdir, $dirsp) = @_;

    $log->debugf('Opening %s', $tarball);
    my $tar = Archive::Tar->new($tarball) || die "Cannot open $tarball, $!";

    my $cwd = getcwd();
    $log->debugf('Moving to %s', $tmpdir);
    chdir($tmpdir) || die "Cannot chdir to $tmpdir, $!";

    $log->debugf('Extracting %s', $tarball);
    $tar->extract();

    $log->debugf('Moving back to to %s', $cwd);
    chdir($cwd) || die "Cannot chdir to $cwd, $!";

    my %dirs = (
	libmarpa_dist => undef,
	libmarpa_doc_dist => undef
	);
    find(
	{
	    no_chdir => 1,
	    wanted => sub {
		if (-d $_) {
		    my $base = basename($_);
		    if (exists($dirsp->{$base})) {
			$dirsp->{$base} = $_;
		    }
		}
	    }
	}, $tmpdir);

    $log->debugf('libmarpa directories paths: %s', \%dirs);

    foreach (keys %{$dirsp}) {
	if (! defined($dirsp->{$_})) {
	    die "Directory $_ not found in $tmpdir";
	}
    }

    return;
}

# #############################################################################
# Process directories
# #############################################################################
sub processDirs {
    my ($dirsp, $newDirsp, $optsp) = @_;

    foreach (keys %{$dirsp}) {
	processDir($_, $dirsp->{$_}, $newDirsp->{$_}, $optsp);
    }
}

# #############################################################################
# Process a directory
# #############################################################################
sub processDir {
    my ($dirName, $dirPath, $newDirName, $optsp) = @_;

    #
    # Rename directories
    #
    my $newDirPath = File::Spec->catdir(dirname($dirPath), $newDirName) . "-$optsp->{libMarpaVersion}";
    $log->debugf('Renaming %s to %s', $dirPath, $newDirPath);
    move($dirPath, $newDirPath) || die "Cannot rename $dirPath to $newDirPath";

    #
    # Debianize ?
    #
    debianize($newDirPath, $optsp) if ($optsp->{debian});

}
