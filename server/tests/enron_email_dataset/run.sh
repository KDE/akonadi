#!/bin/sh

echo "This script retrieves and processes the Enron email dataset to a valid maildir and a set of vcard files."
echo "Most steps involve a considerable amount of time, so it is suggested to find a way to kill time during,"
echo "such as watching a good movie. Have fun!"
echo

# Fetch the Enron email dataset
if [ -f enron_mail_030204.tar.gz ]; then
  echo "Enron email dataset already exists, skipping download."
else
  echo "Downloading Enron email dataset..."
  wget http://www.cs.cmu.edu/~enron/enron_mail_030204.tar.gz
fi

if [ -d maildir ]; then
  echo "Maildir already exists, skipping extraction."
else
  # Extract it
  echo "Extracting dataset..."
  tar zxf enron_mail_030204.tar.gz

  # Transform the dataset to a collection of valid maildirs
  echo "Transforming dataset to valid maildir..."
  php to_valid_maildir.php
fi

# Analyse the email files and extract Name/Company/Email information out of it, write to enron_contacts.txt
mkdir -p vcards
echo "Extracting contacts from dataset..."
php extract_contacts.php

# Remove non-unique contacts
cd vcards
  for i in $(ls);
  do
    cat $i | sort | uniq > ${i}_u;
    mv ${i}_u $i;
  done
cd ..

# Use extracted contacts to create a (fake) addressbook, containing the actual information, but also fake birthdays/telephone numbers/addresses/photos, write to enron_contacts.vcf
echo "Creating VCard files from contacts..."
php create_vcards.php

# Add (fake) attachments to the email dataset (increases maildir size from 2.6GB to 5.2GB with 50,5000,500000 parameters)
echo "Adding attachments to maildir..."
php add_attachments.php --every=50 --minsize=5000 --maxsize=500000

# Convert the created Courier-type maildir to a KMail-type maildir.
echo "Converting the created Courier-type maildir to a KMail-type maildir..."
php courier_to_kmail_maildirformat.php
