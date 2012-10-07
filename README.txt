#BioCreative III Interactive Annotation Task, 3-4GB worth of data with species and entrezgenes annotated

#NEW
GitHub way of doing things, after you have added your ssh key in .git/config

#First, check out the main code repository
git clone git@github.com:saetre/biocreative.git

#Check out 4GB of data (link it as "data" from your biocreative folder)
git clone git@github.com:saetre/biocreative_data.git
cd biocreative
ln -s ../biocreative_data data

#In order to be able to sync changes
git remote add origin git@github.com:saetre/biocreative_data.git

#Check if the config looks ok
cat .git/config


#How to merge (local master) and push changes back to GitHub (origin/master)

#fetch and merge from remote origin/master to local master
git pull git@github.com:saetre/biocreative_data.git   #or "git pull origin master"

#Then (after changing some local files)
git commit
git merge origin/master
git push origin master

#######################################################################

#OLD Tsujiilab SVN way of doing things...
#How to check out remote files, after adding the right entry to
# .ssh/config

svn co svn+ssh://mason10-camplin/home/svn/satre/biocreative biocreative

#List contents
svn list svn+ssh://mason10-camplin/home/svn/satre/biocreative


