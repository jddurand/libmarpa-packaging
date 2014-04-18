#!env perl
use strict;
use diagnostics;
use LWP::Simple;
use File::Temp qw/tempdir/;
use File::Slurp qw/read_file/;
use File::Basename qw/basename/;
use File::Spec;
use File::Copy qw/move/;
use File::Copy::Recursive qw/dircopy/;
use File::Remove qw/remove/;
use File::Find qw/find/;
use File::stat qw/stat/;
use File::chmod qw/chmod/;
use Data::Dumper;
use Getopt::Long;
use Log::Log4perl qw/:easy/;
use Log::Any::Adapter;
use Log::Any qw/$log/;
use URI;
use Archive::Tar;
use Archive::Tar::Constant qw/FILE/;
use Cwd qw/getcwd abs_path/;
use IPC::Run qw/run/;
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
    repgit          => File::Spec->catdir(File::Spec->updir, 'jddurand.github.io'),
    reprepro        => 'reprepro',
    logLevel        => 'INFO',
    debian          => 1,
);
my %cmdOpts = (
    'version=i'         => sub { $opts{libMarpaVersion} = $_[1] },
    'mapFilename=s'     => sub { $opts{mapFilename} = $_[1] },
    'repgit=s'          => sub { $opts{repgit} = $_[1] },
    'reprepro=s'        => sub { $opts{reprepro} = $_[1] },
    'debian!'           => sub { $opts{debian} = $_[1] },
    'help!'             => sub { help(\%opts) },
    'verbose!'          => sub { $opts{logLevel} = $_[1] ? 'DEBUG' : 'WARN' },
    );
GetOptions (%cmdOpts) || die "Error in command line arguments";
$opts{repgit} = abs_path($opts{repgit});

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
    $opts{libMarpaVersion} = (sort {$a =~ s/\.//g; $b =~ s/\.//g; $b <=> $a} keys %map)[0];
}

# ------------
# Load tarball
# ------------
my $tarball = getCPANTarball($opts{libMarpaVersion}, \%map, $TMP_DIRNAME);

# ---------------
# Extract tarball
# ---------------
my %subDirs = (
    libmarpa_dist => undef,
    libmarpa_doc_dist => undef
    );
extractCPANTarball($tarball, $TMP_DIRNAME, \%subDirs);

processDirs(\%subDirs, \%opts);

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

    $log->infof('Loading %s', $mapFilename);

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
                              Default value: 0

--debian                      Debianize.
                              Default value: $optsp->{debian}

--repgit                      git local top directory hosting debian repository.
                              Default value: $optsp->{repgit}

--reprepro                    Reprepro managed repository, relative to repgit.
                              Default value: $optsp->{reprepro}

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

    $log->infof('Loading %s', "$uri");

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

    my $cwd = getcwd();
    $log->debugf('Moving to %s', $tmpdir);
    chdir($tmpdir) || die "Cannot chdir to $tmpdir, $!";

    $log->debugf('Opening %s', basename($tarball));
    my $tar = Archive::Tar->new(basename($tarball)) || die "Cannot open $tarball, $!";

    $log->debugf('Extracting %s', basename($tarball));
    $tar->extract();

    $log->debugf('Moving back to to %s', $cwd);
    chdir($cwd) || die "Cannot chdir to $cwd, $!";

    $log->debugf('Making sure all files are writable in the extracted tarbal');
    find(
	{
	    no_chdir => 1,
	    wanted => sub {
		if (-f $_) {
		    chmod('+w', $_);
		}
	    }
	}, $tmpdir);

    $log->debugf('Looking for %s', [ keys %{$dirsp} ]);
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

    $log->debugf('libmarpa directories paths: %s', $dirsp);

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
    my ($subDirsp, $optsp) = @_;

    #
    # Debianize ?
    #
    debianize(@_) if ($optsp->{debian});

}

# #############################################################################
# Process a directory
# #############################################################################
sub debianize {
    my ($dirsp, $optsp) = @_;

    my %dirsFormat = (
	libmarpa_dist => 'libmarpa-%s',
	libmarpa_doc_dist => 'libmarpa-doc-%s',
	);

    my %tarsFormat = (
	libmarpa_dist => 'libmarpa_%s',
	libmarpa_doc_dist => 'libmarpa-doc_%s',
	);

    my %dirsTemplate = (
	libmarpa_dist => 'libmarpa',
	libmarpa_doc_dist => 'libmarpadoc',
	);

    my %pkgType = (
	libmarpa_dist => 'library',
	libmarpa_doc_dist => 'indep',
	);

    my %pkgName = (
	libmarpa_dist => 'libmarpa',
	libmarpa_doc_dist => 'libmarpa-doc',
	);

    foreach (keys %{$dirsp}) {
	my $dir = $_;

	my $logPrefix = "debian $dir";

	$log->infof('[%s] Processing %s', 'debian', $dir);
	my $revision = "$optsp->{libMarpaVersion}-1";

	my $cwd = getcwd();

	my $tmpDir = tempdir(CLEANUP => 1);
	$log->debugf('[%s] New temporary directory %s', $logPrefix, $tmpDir);
	my $newTopDir = File::Spec->catdir($tmpDir, sprintf($dirsFormat{$dir}, "$optsp->{libMarpaVersion}"));
	#
	# Create newTopDir
	#
	$log->debugf('[%s] Renaming %s to %s', $logPrefix, $dirsp->{$dir}, $newTopDir);
	move($dirsp->{$dir}, $newTopDir) || die "Cannot rename $dirsp->{$dir} to $newTopDir";
	if (0) {
	    #
	    # Move to temporary directory for the archive creation
	    #
	    $log->debugf('[%s] Moving to %s', $logPrefix, $tmpDir);
	    chdir($tmpDir) || die "Cannot chdir to $tmpDir, $!";
	    #
	    # Create orig tarball
	    #
	    my $origTarball = sprintf('%s.orig.tar.gz', sprintf($tarsFormat{$dir}, "$optsp->{libMarpaVersion}"));
	    $log->debugf('[%s] Creating %s', $logPrefix, $origTarball);
	    my $tar = Archive::Tar->new;
	    $tar = Archive::Tar->new();
	    find(
		{
		    no_chdir => 1,
		    wanted => sub {
			my $st = stat($_);
			if ($st && -f $_) {
			    $log->debugf('[%s] FILE %s', $logPrefix, $_);
			    $tar->add_data($_, FILE, {mtime => $st->mtime});
			}
		    }
		},
		basename($newTopDir)
		);
	    $tar->write($origTarball, COMPRESS_GZIP);
	}
	#
	# For doc, the only way to get it properly is to have a libmarpa-doc-x.y.y.orig directory
	#
	if ($dir eq 'libmarpa_doc_dist') {
	    #
	    # Move to temporary directory for the archive creation
	    #
	    my $docDir = File::Spec->catdir($tmpDir, sprintf($dirsFormat{$dir}, "$optsp->{libMarpaVersion}") . '.orig');
	    $log->debugf('[%s] Copying %s to %s', $logPrefix, $newTopDir, $docDir);
	    dircopy($newTopDir, $docDir);
	}
	{
	    #
	    # Move to directory containing source to package
	    #
	    $log->debugf('[%s] Moving to %s', $logPrefix, $newTopDir);
	    chdir($newTopDir) || die "Cannot chdir to $newTopDir, $!";
	    #
	    # Debianize using our templates
	    #
	    my @dh_make = qw/dh_make --copyright lgpl3 --createorig --yes/;
	    push(@dh_make, '--templates', File::Spec->catdir($cwd, 'templates', $dirsTemplate{$dir}, 'debian'));
	    push(@dh_make, '--' . $pkgType{$dir});
	    _system(\@dh_make, $logPrefix);
	    #
	    # Redo the changelog
	    #
	    my $changelog = File::Spec->catfile('debian', 'changelog');
	    if (-e $changelog) {
		$log->debugf('[%s] Unlinking %s', $logPrefix, $changelog);
		unlink($changelog) || $log->warnf('[%s] Cannot unlink %s, %s', $logPrefix, $changelog, $!);
	    }
	    my @dch_changelog = ('dch', '--create', '--package', $pkgName{$dir}, '--newversion', "$revision", "Release of $pkgName{$dir} version $optsp->{libMarpaVersion}");
	    _system(\@dch_changelog, $logPrefix);

	    $log->debugf('[%s] Changing UNRELEASED to unstable in %s', $logPrefix, $changelog);
	    my $changelogContent = read_file($changelog);
	    $changelogContent =~ s/\bUNRELEASED\b/unstable/;
	    open(CHANGELOG, '>', $changelog) || die "Cannot open $changelog, $!";
	    print CHANGELOG $changelogContent;
	    close(CHANGELOG) || $log->warnf('[%s] Cannot close %s, %s', $logPrefix, $changelog, $!);

	    #
	    # remove /.ex/i files
	    #
	    $log->debugf('[%s] Removing /\\.ex$/i files in %s directory', $logPrefix, 'debian');
	    find(
		{
		    no_chdir => 1,
		    wanted => sub {
			if (-f $_ && $_ =~ /\.ex$/i) {
			    unlink($_) || $log->warnf('[%s] Cannot remove %s, %s', $logPrefix, $_, $!);
			}
		    }
		}, 'debian');
	    #
	    # remove other files
	    #
	    foreach (File::Spec->catfile('debian', 'README.Debian'), 'COPYING.LESSER') {
		$log->debugf('[%s] Removing %s', $logPrefix, $_);
		unlink($_) || $log->warnf('[%s] Cannot remove %s, %s', $logPrefix, $_, $!);
	    }
	}
	{
	    #
	    # Move to temporary directory for the archive creation
	    #
	    $log->debugf('[%s] Moving to %s', $logPrefix, $tmpDir);
	    chdir($tmpDir) || die "Cannot chdir to $tmpDir, $!";
	    #
	    # Create debian tarball
	    #
	    my $debianTarball = sprintf('%s.debian.tar.gz', sprintf($tarsFormat{$dir}, "$revision"));
	    $log->debugf('[%s] Creating %s', $logPrefix, $debianTarball);
	    my $tar = Archive::Tar->new;
	    $tar = Archive::Tar->new();
	    find(
		{
		    no_chdir => 1,
		    wanted => sub {
			my $st = stat($_);
			if ($st && -f $_) {
			    $log->debugf('[%s] FILE %s', $logPrefix, $_);
			    $tar->add_data($_, FILE, {mtime => $st->mtime});
			}
		    }
		},
		File::Spec->catdir(basename($newTopDir), 'debian')
		);
	    $tar->write($debianTarball, COMPRESS_GZIP);
	}
	{
	    #
	    # Move to directory containing source to package
	    #
	    $log->debugf('[%s] Moving to %s', $logPrefix, $newTopDir);
	    chdir($newTopDir) || die "Cannot chdir to $newTopDir, $!";
	    #
	    # Build the package
	    #
	    my @debuild = ('debuild', '-us', '-uc');
	    _system(\@debuild, $logPrefix);
	}
	{
	    $log->debugf('[%s] Moving to %s', $logPrefix, $optsp->{repgit});
	    chdir($optsp->{repgit}) || die "Cannot chdir to $optsp->{repgit}, $!";
	    $log->debugf('[%s] Looking for .deb and .desc for reprepro inclusion', $logPrefix, $cwd);
	    my $reppath = File::Spec->catdir($optsp->{repgit}, $optsp->{reprepro});
	    find(
		{
		    no_chdir => 1,
		    wanted => sub {
			if (/\.(deb|dsc)$/) {
			    my @remove = ('reprepro', '-Vb', $reppath, 'remove', 'unstable', $pkgName{$dir});
			    _system(\@remove, $logPrefix);
			    my @include = ('reprepro', '-Vb', $reppath, "include$1", 'unstable', $_);
			    _system(\@include, $logPrefix);
			}
		    }
		},
		$tmpDir
		);
	}

	$log->debugf('[%s] Moving back to %s', $logPrefix, $cwd);
	chdir($cwd) || die "Cannot chdir to $cwd, $!";
    }
}

sub _system {
    my ($cmdp, $logPrefix, $in) = @_;

    $logPrefix //= '';
    $logPrefix = "[$logPrefix] " if ($logPrefix);

    if ($in) {
	$log->debugf('%sExecuting command %s with input: %s', $logPrefix, $cmdp, $in);
    } else {
	$log->debugf('%sExecuting command %s', $logPrefix, $cmdp);
    }
    run($cmdp,
	\$in,
	sub {
	    foreach (split(/\n/, $_[0])) {
		$log->infof('%s=> %s', $logPrefix, $_);
	    }
	},
	sub {
	    foreach (split(/\n/, $_[0])) {
		$log->errorf('%s=> %s', $logPrefix, $_);
	    }
	}
	)
	||
	die "@{$cmdp}: $!";
}
