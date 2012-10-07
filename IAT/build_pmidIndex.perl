#!/usr/bin/perl
#
#SCRIPT: build_pmidIndex.perl
#
#Reads the index file on this form
# <PMCID> \t <GENE-ID> \t <FROM>-<TO> <FROM>-<TO> <FROM>-<TO> ...
# 13920	EntrezGene:17979|MGI:1276535|SWISS-PROT:O09000	145-149 449-453
# 13920 ...
#
#and the mappingfile on this form
# <PMID>,PMC<PMCID>,
#
# And builds 
# <PMID> \t genecount ...
# 

use strict;

#my $folder = ''; #'/works/mlesna3a/medie-admin/data-2009';

#FILES:
my $goldfile = 'index.txt';
my $mappingfile = 'annotations.txt';
my $indexfile = '/works/typhoo/satre/BC3/data/IAT/pmidIndex.txt';

#GLOBAL VARIABLES
my %pmcid2pmid = ();
getPmidMapping($mappingfile, \%pmcid2pmid);

my %pmids = ();
#returns pmids as hash{PMID->#mentions}
getPmids($goldfile, \%pmids, \%pmcid2pmid);

print STDERR "Write output to $indexfile...\n\n";
open(OUTFILE,">$indexfile") or usage("Cannot write to <$indexfile>\n\n");
print OUTFILE "#Generated on ".`date`;
print OUTFILE "#by ~satre/public_html/biocreative/data/IAT/build_geneIndex.perl\n";
print OUTFILE "#from $goldfile\n";
print OUTFILE "#and from $mappingfile\n";
print OUTFILE "#PMID\t\tfreq-in-abstract\n";
foreach my $pmid (sort keys %pmids){
	print  "$pmid\t$pmids{$pmid}\n";
	my $line = "$pmid\t$pmids{$pmid}\n";
	print OUTFILE "$line";
}#foreach gene-line
close(OUTFILE);


#################
# SUB-FUNCTIONS #
#################

sub getPmidMapping{
	my ($goldfile, $pmcid2pmid_r) = @_;
	open (GOLDFILE, $goldfile) or usage ("Cannot open goldfile <$goldfile>\n");
	foreach my $line (<GOLDFILE>){
		my ($pmid,$pmcid,$rest) = split /,/, $line;
		$pmcid =~ s/^PMC//;
		$pmcid2pmid_r->{$pmcid} = $pmid;
	}
	close GOLDFILE;
}#sub getPmidMapping

#Get the list of pmids to be included in the index, from
#13920	EntrezGene:17979|MGI:1276535|SWISS-PROT:O09000	145-149 449-453
sub getPmids{
	my ($goldfile, $pmids_r, $pmcid2pmid_r) = @_;
	print STDERR "file: $goldfile, ";
	open (GOLDFILE, $goldfile) or usage ("Cannot open goldfile <$goldfile>\n");
	my @buffer = <GOLDFILE>;
	close GOLDFILE;
	foreach my $line (@buffer){
		my ($pmcid,$gene,$rest) = split /\t/, $line;
		#my @rest = split /\s/, $rest;
		#if ($gene =~ /EntrezGene:(\d+)/){
			#print "$1\t";
		#	$gene = $1;
		$pmids_r->{ $pmcid2pmid_r->{$pmcid} }++;
		#}else{
		#	#print STDERR "MISSING EntrezGene is $gene\n";
		#}
	}#foreach pmid
	my $teller = scalar keys %{$pmids_r};
	print STDERR "Total $teller Gold Entrez genes...\n";
}#sub getPmids

sub usage{
	print "\n\n$_[0]\n";
	print "Usage: ./build_pmidIndex.perl <INPUTFILES>\n";
	die "E.g.:\n ./build_pmidIndex.perl /works/mlesna3a/medie-admin/data-2009/*/*.bib.so \n\n";
}#sub usage


print "\n\n";
