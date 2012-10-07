#!/usr/bin/perl
#
#SCRIPT: build_geneIndex.perl
#
#Reads the index.txt file on this form (PMID, MEDIE)
# <PMID> \t <GENE-ID> \t <FROM>-<TO> <FROM>-<TO> <FROM>-<TO> ...
# 11809893	medline09n0456	3496441	3499299
#or this form (PMC, GNSUITE)
# <PMCID> \t <GENE-ID> \t <CONF> \t <FROM>-<TO> <FROM>-<TO> <FROM>-<TO> ...
# 11809893	medline09n0456	3496441	3499299
#
# Which leads to
# <GENE-ID> \t <PMID>(title,full-count) <PMID>(title,full-count) <PMID> ...
# 

use strict;

#$mode="pmid"; #No <CONF> in the file;
my $mode="pmcid"; #With <CONF> in the file;


#FILES:
my $goldfile = 'GNSuite.complete.txt.small';
my $goldfile = 'GNSuite.complete.txt';
my $folder = 'results'; #'/works/mlesna3a/medie-admin/data-2009';
my $indexfile = '/home/users/satre/public_html/biocreative/data/IAT/geneIndex.GNSuite.txt.new';

#GLOBAL VARIABLES
my %goldgenes = ();


#returns goldgenes as hash{entrezgeneID}->hash{PMID}->#mentions
getGoldGenes("$folder/$goldfile", \%goldgenes);

print STDERR "Write output to $indexfile...\n\n";
open(OUTFILE,">$indexfile") or usage("Cannot write to <$indexfile>\n\n");
print OUTFILE "#Generated on ".`date`;
print OUTFILE "#by ~satre/public_html/biocreative/data/IAT/build_genePmcIndex.perl\n";
print OUTFILE "#from $goldfile\n";
print OUTFILE "#EntrezGene-ID \t PMCID(freq) \t PMCID(freq) \t ...\n";
foreach my $gene (sort {$a <=> $b} keys %goldgenes){ #numeric sort of Entrez-IDs
	my $line = "$gene";
	#my $pmids_r = $goldgenes{$gene};
	foreach my $pmid (sort keys %{$goldgenes{$gene}}){
		print  "$gene\t$pmid($goldgenes{$gene}{$pmid})\n";
		$line .= "\t$pmid($goldgenes{$gene}{$pmid})";
	}#foreach gene-line
	print OUTFILE "$line\n";
}#foreach gold pmid
close(OUTFILE);


#################
# SUB-FUNCTIONS #
#################

#Get the list of genes to be included in the index
# ...goldgenes {$gene} ==> {$pmid} ==> frequency
# Frequency from (22-26 326-330 432-436) or 0.29	6:596-629 7:1343-1376 30:325-371
sub getGoldGenes{
	my ($goldfile, $goldgenes_r, $mode) = @_;
	print STDERR "file: $goldfile, ";
	open (GOLDFILE, $goldfile) or usage ("Cannot open goldfile <$goldfile>\n");
	my @buffer = <GOLDFILE>;
	close GOLDFILE;
	foreach my $line (@buffer){
		if ($mode=="pmcid"){
			my ($pmid,$gene,$conf,$rest) = split /\t/, $line;
			my @rest = split /\s/, $rest;
			if ($gene =~ /(\d+)/){
				print "$1\t";
				$gene = $1;
				my $titleCount = getTitleCountGNSuite(@rest);
				#$goldgenes_r->{$gene}{$pmid} = scalar @rest;
				$goldgenes_r->{$gene}{$pmid} = scalar @rest.",$titleCount";
			}else{
				print STDERR "MISSING EntrezGene is $gene\n";
			}
		}else{
			my ($pmid,$gene,$rest) = split /\t/, $line;
			my @rest = split /\s/, $rest;
			if ($gene =~ /EntrezGene:(\d+)/){
				#print "$1\t";
				$gene = $1;
				$goldgenes_r->{$gene}{$pmid} = scalar @rest;
			}else{
				#print STDERR "MISSING EntrezGene is $gene\n";
			}
		}
	}#foreach pmid
	my $teller = scalar keys %{$goldgenes_r};
	print STDERR "Total $teller Gold Entrez genes...\n";
	return 
}#sub getGoldGenes

sub getTitleCountGNSuite{
	my @offsets = @_;
	my $titleCount = 0;
	if ($offsets[0] =~ /^0/){
		$titleCount++;
	}
	return $titleCount;
}#sub getTitleCountGNSuite

sub getTitleCountMEDIE{
	my @offsets = @_;
	my $titleCount = 0; my $titleLength = 100; # Maybe??!??
	if ($offsets[0] <$titleLength){
		$titleCount++;
	}
	return $titleCount;
}#sub getTitleCountMEDIE

sub usage{
	print "\n\n$_[0]\n";
	print "Usage: ./build_geneIndex.perl <INPUTFILES>\n";
	die "E.g.:\n ./build_geneIndex.perl /works/mlesna3a/medie-admin/data-2009/*/*.bib.so \n\n";
}#sub usage


print "\n\n";
