#!/usr/bin/perl
#
# purpose: Download google/bing images for every landable waypoint and add
#          these images to a waypoint details file. Existing images will
#          not be downloaded again.
# input: see 'usage'
# output: - waypoint details on stdout (to be piped into a file)
#         - images are in the subdirectory 'google' or 'bing'
#         - errors are appended to errors.txt

sub usage
{
  my ($message) = @_;

  print STDERR <<USAGE
Error: $message

usage: $0 <PROVIDER> <CUP-FILE> [<WP-DETAILS-INPUT>]

The use of this script and the use of the data obtained with
this script is subject to legal restrictions and even might be
illegal. Use at your own risk after carefully studying the
respective terms of use.

LEGAL RESTRICTIONS
  google: http://code.google.com/apis/maps/terms.html
  bing: http://www.microsoft.com/maps/assets/docs/terms.aspx

TECHNICAL RESTRICTIONS
  google: download quota max. 250images/6h, max. 50images/min
  bing: often no image data available

<PROVIDER> is either google or bing
<CUP-FILE> waypoints for which images are downloaded (using only type 2-5)
<WP-DETAILS-INPUT> optional existing waypoint details to be extended
USAGE
;
  exit 1;
}

if ($#ARGV != 1 && $#ARGV != 2) {
  usage('Wrong number of arguments!');
}
if ($ARGV[0] ne 'google' && $ARGV[0] ne 'bing') {
  usage('Invalid provider!');
}

$provider = $ARGV[0];
# number of downloading errors till downloading is disabled
$errors_allowed = 1;
# minimum image size in bytes; smaller images are considered invalid
$min_image_size = 10000;
# image size
$width = 512;
$height = 512;

# read existing waypoint details
if ($#ARGV == 2) {
  open FILE, $ARGV[2] or die("Error: Could not open '${ARGV[2]}' for reading\n");
  $waypoint = '';
  while (<FILE>) {
    chomp;
    $line = $_;
    if ($line =~ /^\[([^\]]+)\]/) {
      $waypoint = lc($1);
      $details{$waypoint} = "[$1]\n";
    } else {
      $waypoint ne '' or die("invalid waypoint details file\n");
      $details{$waypoint} .= $line . "\n";
    }
  }
  close FILE;
}

# get the images
mkdir $provider;
open FILE, $ARGV[1] or die("Error: Could not open '${ARGV[1]}' for reading\n");
open ERRLOG, '>>errors.txt';
while ($line = <FILE>) {
  if ($line =~ /^"([^"]+)",[^,]*,[^,]*,\s*(\d+)(\d{2}\.\d*)(N|S)\s*,\s*(\d+)(\d{2}\.\d*)(E|W)\s*,[^,]*,\s*(\d+)\s*,/) {
    $wp_name = $1;
    $wp_idx = lc($wp_name);
    $lat = ($2 + $3/60.0) * ($4 eq 'N' ? 1 : -1);
    $lon = ($5 + $6/60.0) * ($7 eq 'E' ? 1 : -1);
    $wp_type = $8;
    $image_name = $wp_name;
    $image_name =~ s/[^0-9a-zA-Z]/_/g;  # avoid charset hassle in filenames
    $image_name = "$provider/$image_name" .
                  ($provider eq 'google' ? '.png' : '.jpg');
    if ($wp_type >= 2 && $wp_type <= 5) {
      if (-e $image_name) {
        $res = 0;  # success
      } elsif ($errors_allowed > 0) {
        do {
          sleep(int(rand 5) + 1);  # avoid too much server stress
          $cmd = "wget --retry-connrefused".
                 ($provider eq 'google' ?
                   " \"http://maps.googleapis.com/maps/api/staticmap?center=$lat,$lon&zoom=14&size=${width}x$height&maptype=hybrid&markers=color:green|size:small|$lat,$lon&sensor=false\"" :
                   " \"http://www.bing.com/local/GetMap.ashx?c=$lat,$lon&z=14&w=$width&h=$height&b=h\"").
                 " -O $image_name";
          $res = system($cmd);
          if ($res != 0) {
            print ERRLOG "$res $cmd\n";
            unlink $image_name;
            $errors_allowed--;
          }
        } while ($res != 0 && $errors_allowed > 0);
      } else {
        $res = 1;  # error
      }

      if ($res == 0 && -s $image_name > $min_image_size) {
        # add image reference to waypoint details
        if (!exists($details{$wp_idx})) {
          $details{$wp_idx} = "[$wp_name]\n";
        }
        $details{$wp_idx} .= "image=$image_name\n";
      }
    }
  }
}
close ERRLOG;
close FILE;

# output waypoint details
while (($waypoint, $text) = each %details) {
  print $text;
}
