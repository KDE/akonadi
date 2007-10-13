#!/bin/sh

# Fetch the Enron email dataset
wget http://www.cs.cmu.edu/~enron/enron_mail_030204.tar.gz
# Extract it
tar zxvf enron_mail_030204.tar.gz

# Transform the dataset to a collection of valid maildirs
php to_valid_maildir.php

# Analyze the email files and extract Name/Company/Email information out of it, write to enron_contacts.txt
mkdir -p vcards
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
php create_vcards.php

# Add (fake) attachments to the email dataset
php add_attachments.php --chance=18 --minsize=5000 --maxsize=5000000
