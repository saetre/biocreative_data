#BioCreative III Interactive Annotation Task, 3GB data with species and entrezgenes

#NEW
GitHub way of doing things, after you have added your ssh key in .git/config

#First, check out the main code repository
git clone git@github.com:saetre/biocreative.git

#Check out 4GB of data (put it in your biocreative folder)
git clone git@github.com:saetre/biocreative_data.git

#In order to be able to sync changes
git remote add origin git@github.com:saetre/biocreative_data.git

#How to check if the config looks ok
cat .git/config


#######################################################################

#OLD Tsujiilab SVN way of doing things...
#How to check out remote files, after adding the right entry to
# .ssh/config

svn co svn+ssh://mason10-camplin/home/svn/satre/biocreative biocreative

#List contents
svn list svn+ssh://mason10-camplin/home/svn/satre/biocreative


