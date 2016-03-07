#!/bin/bash

DIR="$( cd "$(dirname "$0")" ; pwd -P )"
cd $DIR

FILE_DOC=docword.nytimes.txt
FILE_DOC_GZ=$FILE_DOC.gz
FILE_VOCAB=vocab.nytimes.txt
URL=https://archive.ics.uci.edu/ml/machine-learning-databases/bag-of-words
URL_DOC=$URL/$FILE_DOC_GZ
URL_VOCAB=$URL/$FILE_VOCAB
CHECKSUM_DOC=002df04abf9ff9edf3b9a66bc5d95b81
CHECKSUM_VOCAB=4adeed32e0ec59c538360f9fbdb7c3f8

if [ -f $FILE_DOC ]; then
  echo "File \"$FILE_DOC\" already exists. Checking md5..."

  checksum=`md5sum $FILE_DOC | awk '{ print $1 }'`

  if [ "$checksum" = "$CHECKSUM_DOC" ]; then
    echo "Checksum is correct. No need to download."
  else
    echo "Checksum is incorrect. Need to download again."
    echo "Downloading $FILE_DOC_GZ"
    wget $URL_DOC -O $FILE_DOC_GZ
    echo "Unzipping..."
    gzip -d $FILE_DOC_GZ
  fi
else
  echo "Downloading $FILE_DOC_GZ"
  wget $URL_DOC -O $FILE_DOC_GZ
  echo "Unzipping..."
  gzip -d $FILE_DOC_GZ
fi


if [ -f $FILE_VOCAB ]; then
  echo "File \"$FILE_VOCAB\" already exists. Checking md5..."
  checksum=`md5sum $FILE_VOCAB | awk '{ print $1 }'`
  if [ "$checksum" = "$CHECKSUM_VOCAB" ]; then
    echo "Checksum is correct. No need to download."
    exit 0
  else
    echo "Checksum is incorrect. Need to download again."
  fi
fi

echo "Downloading $FILE_VOCAB"

wget $URL_VOCAB -O $FILE_VOCAB

echo "Done."
