#!/usr/bin/perl
#
#SCRIPT: build_index.perl
#
#Reads the pubmed IDs and offsets from
# $folder
#and constructs an index file on this form <PMID> \t <FOLDER> \t <FROM> \t <TO>
# 11809893	medline09n0456	3496441	3499299
#
# Which leads to
#
# 2684697	EntrezGene:19645|MGI:97874|SWISS-PROT:P13405|TrEMBL:Q9CT40	517-520 920-923 1084-1087
 
#USAGE
#	E.g.: ./build_index.perl /works/mlesna3a/medie-admin/data-2009/med*/*.txt-tbl.gz

use strict;


#FILES:
my $mappingfile = 'annotations.txt'; #PMID <> PMCID mapping
my $folder = ''; #'/works/mlesna3a/medie-admin/data-2009';
my $filemask = 'medline09n*';
my $indexfile = '/works/typhoo/satre/BC3/data/IAT/index.txt';
my $sourcefiles = 'abstracts.txt';
#*.bib.so files look like this:
#1       1       bib Article.Journal.ISSN="0014-4819"    PMID="11807580"
#
#*.txt-tbl.gz files look like this
#PMID 		medline09n0xxx 	from		to			titlefrom to		abstractfrom to
#19513100 medline09n0817  3133043 3135304 3133135 3133243 3133256 3134281

my $outputfolder = 'pmcid_genes';

#GLOBAL VARIABLES
my %goldpmids = ();
my %sourcefiles = ();

getGoldPMIDS($mappingfile, \%goldpmids);

if (!@ARGV){
	usage();
}

my %index = ();
my $filecounter = 0;
while (@ARGV){
	my $infile = shift @ARGV;
	#print "\ncurrent file: $infile\n\n";
	my @genes = getAllGenesFromFile($infile);

	#index is pmcid -> ids -> offsets
	getIndex($infile, \%goldpmids, \%index, \@genes, \%sourcefiles);

	if ($filecounter++ %100 ==0){
		print "\n$filecounter";
	}
	#print STDERR ".";
}#for each input-file

open(OUTFILE,">$indexfile") or usage("Cannot write to <$indexfile>\n\n");
foreach my $key (sort {$a <=> $b} keys %index){ #numeric sort of PMC-IDs
	if ($index{$key}){
		my %ids = %{$index{$key}};
		foreach my $gene (keys %ids){
			#print "$key\t$gene\t$ids{$gene}\n";
			print OUTFILE "$key\t$gene\t$ids{$gene}\n";
		}#foreach gene-line
	}else{ #MISSING genes for id = $key
		#print "$key\t\t\n";
		print OUTFILE "$key\t\t\n";
	}
}#foreach gold pmid
close(OUTFILE);

open(OUTFILE,">$sourcefiles") or usage("Cannot write to <$sourcefiles>\n\n");
foreach my $key (sort {$a <=> $b} keys %sourcefiles){ #numeric sort of PMC-IDs
	if ($sourcefiles{$key}){
		print OUTFILE "$key\t$sourcefiles{$key}";
	}else{ #MISSING genes for id = $key
		#print "$key\t\t\n";
		print OUTFILE "$key\t\t\n";
	}
}#foreach gold pmid
close(OUTFILE);


#################
# SUB-FUNCTIONS #
#################

sub getAllGenesFromFile{
	my $genesfile = shift @_;
	#$genesfile =~ s/.txt-tbl.gz/.gene-ne.so/; #HUMAN ONLY!
	$genesfile =~ s/.txt-tbl.gz/.gene-ne-all.so/;
	#open (GENEFILE, "$genesfile") or usage("Cannot open Genesfile <$genesfile>\n");
	#my @genes = <GENEFILE>;
	#close (GENEFILE);
	my @genes = `zcat $genesfile` or  usage("Cannot open Genesfile <$genesfile>\n");
	return @genes;
}#getAllGenesFromFile


#Get the list of pmids to be included in the index
sub getGoldPMIDS{
	my ($mappingfile, $goldpmids_r) = @_;
	print STDERR "file: $mappingfile, ";
	open (GOLDFILE, $mappingfile) or usage ("Cannot open goldfile <$mappingfile>\n");
	my @buffer = <GOLDFILE>;
	close GOLDFILE;
	foreach my $line (@buffer){
		my ($pmid,$pmc) = split /,/, $line;
		$goldpmids_r->{$pmid} = $pmc;
		#print STDERR "$pmid\t";
	}#foreach pmid
	my $teller = scalar keys %{$goldpmids_r};
	print STDERR "Total $teller Gold pmids...\n";
	return 
}#sub getGoldPMIDS

sub getIndex{
	my ($infile, $goldpmids_r, $index_r, $genes_r, $source_r) = @_;  # or die "Index not provided...";
	my $textfile = $infile; $textfile =~ s/.txt-tbl.gz/.txt/;
	my $text = "";

	#Get the (starting, and ending) offsets for each pmid in the given bib-file
	my @buffer = `zcat $infile` or usage("Cannot open offsetfile <$infile>\n\n");
	my $teller = scalar @buffer;
	#print STDERR "file: $infile, Contains $teller pmids...\n";

	my ($pmid, $file, $from, $to, $tFrom, $tTo, $titleSize, $aFrom, $aTo, $titleGap);
	#my ($prev_from, $prev_to, $prev_titleSize, $prev_titleGap, $prev_issn, $prev_pmid);
	#my $output = 0;
	foreach my $line (@buffer){
		chomp $line;
		($pmid, $file, $from, $to, $tFrom, $tTo, $aFrom, $aTo) = split /\t/, $line;
		#$pmid =~ s/PMID="(\d+)"/$1/;

		if ( $goldpmids_r->{$pmid} ){
			my $pmcid = $goldpmids_r->{$pmid}; $pmcid =~ s/PMC//;
			addGene($index_r, $genes_r, $source_r, $tFrom, $aTo, $pmcid, $textfile);
		}else{
			#print "SKIP:$pmid\t";
		}
		#($prev_from, $prev_to, $prev_issn, $prev_pmid) = ($from, $to, $issn, $pmid);
	}#foreach pmid
}#sub getIndex



#index is pmcid -> ids -> offsets
sub addGene{
	my ($index_r, $genes_r, $sourcefiles_r, $art_from,$art_to, $pmid, $textfile) = @_;
	$index_r->{$pmid} = ();
	
	#print STDERR "$pmid\t$textfile\t$art_from\t$art_to\n";
	$sourcefiles_r->{$pmid} = "$textfile\t$art_from\t$art_to\n";

	my $line = shift @{$genes_r};
	if ($line){
		my ($from, $to, $tag, $ids) = split /\t/, $line;
		while ($line && $from<$art_to){
			if ($from>=$art_from){
				$from -= $art_from; $to -= $art_from;
				$ids =~ s/^.*db_site="([^"]*).*\n$/$1/;
				#$text .= "$from $to $ids";
				if ( $index_r->{$pmid}{$ids} ){
					$index_r->{$pmid}{$ids} .= " $from-$to";
				}else{
					$index_r->{$pmid}{$ids} = "$from-$to";
				}#if first occurrence
			}#if current article
			$line = shift @{$genes_r};
			($from, $to, $tag, $ids) = split /\t/, $line;
		}#while more possible tags
		#Put last tag back again
		if ($line){
			unshift (@{$genes_r}, $line);
		}
	}#if not last line
}#sub addGene

sub usage{
	print "\n\n$_[0]\n";
	print "Usage: ./build_index.perl <INPUTFILES>\n";
	die "E.g.:\n ./build_index.perl /works/mlesna3a/medie-admin/data-2009/med*/*.txt-tbl.gz \n\n";
}#sub usage


print "\n\n";
