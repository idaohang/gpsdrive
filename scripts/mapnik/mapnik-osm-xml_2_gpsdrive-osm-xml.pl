#!/usr/bin/perl

use strict;
use warnings;

use XML::Twig;
use IO::File;
use Data::Dumper;
use Image::Info;

my $filename_in = "/home/tweety/svn.openstreetmap.org/applications/rendering/mapnik/osm.xml";
my $filename_out ="osm-copy.xml";

my $t= XML::Twig->new(
    twig_handlers =>
    { 
#	Rule => \&rule,
#	Layer => \&layer,
	Parameter => \&parameter,
	LinePatternSymbolizer => \&Symbolizer,
	PointSymbolizer => \&Symbolizer,
	PolygonPatternSymbolizer => \&Symbolizer,
	ShieldSymbolizer => \&Symbolizer,
	TextSymbolizer => \&TextSymbolizer,
    },
    pretty_print => 'indented',                # output will be nicely formatted
    keep_atts_order => 1,
    );
$t->parsefile( $filename_in);
open( OUT,">osm-copy.xml");
#$t->flush;
print OUT $t->sprint(1);                            # output the document

# Get Size from a PNG
sub get_png_size($){
    my $filename = shift;
    my ($s1,$s2)=Image::Info::image_info($filename);
#    print "S1: ".Dumper(\$s1);
#    print "S2: ".Dumper(\$s2);
#    my $comment = $s1->{'Comment'};
    return($s1->{'height'},$s1->{'width'});
}


sub TextSymbolizer {
    my( $t, $rule)= @_;      # arguments for all twig_handlers
    my $face_name = $rule->att('face_name');
    $face_name =~ s/DejaVu Sans Bold/DejaVu Sans Book/;
    $rule->set_att('face_name',$face_name);
}
sub Symbolizer {
    my( $t, $rule)= @_;      # arguments for all twig_handlers
    my $file = $rule->att('file');
#    $rule->{'att'}->{'file'} =~ s,/home/steve/symbols,\@ICON_INSTALL_DIR\@/classic.small,;
    $file =~ s,/home/steve/symbols/(.*),\@ICON_INSTALL_DIR\@/symbols/$1,;   
    my $png=$1;
    if ( $png ) {
	my $search = $png;
	# XXX: These are HACKS, please check if they are right
	$search =~ s/station\.png/railway.png/;
#	$search =~ s/station_small.png/railway_small.png/;
	$search =~ s/bus_stop.png/bus.png/;
	$search =~ s/place_of_worship.png/church.png/;
#	$search =~ s/mini_round.png/roundabout_left.png/;
	$search =~ s/grave_yard.png/cemetery.png/;
#	print `find ../../data/map-icons/classic.small/ -name $search`;
	my $png_path=`find ../../data/map-icons/classic.small/ -name $search | grep -v incomming | head -1`;
	chomp $png_path;
	if ( -s $png_path ) {
	    my ($h,$w) = get_png_size($png_path);
	    $png_path =~ s,.*/classic.small/,classic.small/,;
	    print "Searching for '$png' --> $png_path  ($h,$w)\n";
	    
	    $file =~ s,symbols/$png,$png_path,;
	    $rule->set_att('file',$file);
	    $rule->set_att('height',$h) if $h;
	    $rule->set_att('width',$w) if $w;
	} else {
	    print "!!!!!!!! WARNING: File $png not found\n";
	}
	$rule->set_att('file',$file);
    }

    my $face_name = $rule->att('face_name');
    if ( $face_name ) {
	$face_name =~ s/Book/Bold/;
	$rule->set_att('face_name',$face_name);
    }

    $Data::Dumper::Maxdepth=3;
#    print "Symbolizer: ".Dumper(\$rule);
}

sub parameter {
    my( $t, $rule)= @_;      # arguments for all twig_handlers
    my $attr='name';
    my $val= $rule->{'att'}->{$attr}; # get the attribute
#    print "parameter ==> keys(att->): " . join(",",keys %{$rule->{'att'}})."\n";
#    print "parameter ==> att->$attr=$val\n";
    
    $Data::Dumper::Maxdepth=3;
    my $name = $rule->{'att'}->{'name'};
    if ( $name eq 'file' ) {
#	my $child = $rule->first_child( );
	$rule->subs_text (qr{/home/steve/world_boundaries}, '@WORLD_BOUNDARIES_INSTALL_DIR@');
	$rule->subs_text (qr{/home/steve/symbols/}, '@DATA@');
	$rule->subs_text (qr{/home/steve/}, '@DATA@/');
#	print "parameter: ".Dumper(\$rule);
    } elsif ( $name eq 'host' ) {
	my $child = $rule->first_child( );
	$rule->subs_text (qr{dev.openstreetmap.org},'/var/run/postgresql');
    } elsif ( $name eq 'port' ) {
	my $child = $rule->delete();
    } elsif ( $name eq 'user' ) {
	$rule->subs_text (qr{steve},'@USER@');
    } elsif ( $name eq 'dbname' ) {
	$rule->subs_text (qr{steve},'gis');
    } else {
	#print "parameter: ".Dumper(\$rule);
    };
}

sub rule {
    my( $t, $rule)= @_;      # arguments for all twig_handlers
    my $symbolizer;
    if ( $symbolizer = $rule->first_child( 'TextSymbolizer')) {
	my $attr='face_name';
	my $val= $symbolizer->{'att'}->{$attr}; # get the attribute
	print "rule ==> $attr: $val\n";
#      $val = "DejaVu Sans Book";
	$symbolizer->{'att'}->{$attr}=$val; # set the attribute
    }

    if ( $symbolizer = $rule->first_child( 'LinePatternSymbolizer')) {
	my $attr='file';
	if (defined ( $symbolizer->{'att'} ) && 
	    defined ( $symbolizer->{'att'}->{$attr} ) ) {
	    my $val= $symbolizer->{'att'}->{$attr}; # get the attribute
	    print "rule ==> $attr: $val\n";      
	    $val =~ s,/home/steve/symbols/,\@DATA_DIR\@/map-icons/classic.small/,;
	    $val =~ s,/home/steve/world_boundaries/,\@DATA_DIR\@/mapnik/world_boundaries/,;
	    $symbolizer->{'att'}->{$attr}=$val; # set the attribute
	}
    }
}