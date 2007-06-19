<?php

/***************************************************************************
 *   Copyright (C) 2007 by Robert Zwerus <arzie@dds.nl>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

	/*
		CLI arguments:
		- chance of an attachment
		- minimum/maximum size of attachment

		open maildir
			scan all users
				scan all subdirs
					if(rand(0,100) < chance)
						open file
						add attachment
							add mime-stuff (content-type: multipart/mixed, part separators)
	  					generate random data (size: rand(minimumsize,maximumsize) and base64_encode it
						write file

	*/
// FIXME: implement pseudocode above
exit();

// The output file and its header
$output = fopen("enron_contacts.txt", "w");
fwrite($output, "Name;Company;Email");

// The mail headers to look for
$search_elements = array("X-To:", "To:", "X-From:", "From:"); //, "Cc:", "Bcc:");

// Location of maildirs
$maildirname = "maildir";

$maildir = opendir($maildirname);
$c = 0;

// Process all mail accounts
while($account = readdir($maildir))
{
  // Skip current and parent directory
  if($account == "." || $account == "..")
    continue;

  // Open directory with all documents
  $alldocsname = $maildirname . "/" . $account . "/" . "all_documents";
  if($alldocs = @opendir($alldocsname))
  {
    echo "Analyzing account '" . $account . "'...";

    $contacts = array();

    // Read next mail file
    while($mailfile = readdir($alldocs))
    {
      $c++;

      // Open mail file
      $fd = fopen($alldocsname . "/" . $mailfile, "r");
      if($fd)
      {
        $values = array();

        // Read file line by line
        while($line = fgets($fd))
        {
          // Stop after headers
          if($line == "\n")
            break;

          // Remove trailing newline
          $line = substr($line, 0, strlen($line) - 1);

          // Search for certain headers
          foreach($search_elements as $search_element)
          {
            $length = strlen($search_element);

            // Skip headers without contents
            if(strlen($line) > ($length + 3))
            {
              // Check if header matches one of the searched headers
              if(substr_compare($line, $search_element, 0, $length) == 0)
              {
                $values[$search_element] = explode_nicely(",;", substr($line, $length + 1));
              }
            }
          }
        }
        // Interpret headers
        for($i = 0; $i < count($values["X-From:"]); $i++)
        {
          $contact = interprete_values($values["X-From:"][$i], $values["From:"][$i]);
          $contacts[$contact[2]] = $contact;

        }
        for($i = 0; $i < count($values["X-To:"]); $i++)
        {
          $contact = interprete_values($values["X-To:"][$i], $values["To:"][$i]);
          $contacts[$contact[2]] = $contact;
        }
      }
    }

    // Write headers to output file
    foreach($contacts as $contact)
    {
      $line = implode(";", $contact) . "\n";
      if($line != "\n")
      {
        fwrite($output, $line);
//        echo($line);
      }
    }

    echo " " . count($contacts) . " contacts found.\n";
  }
}

echo "Number of attachments added: " . $c . "\n";
closedir($maildir);
exit();

function random_string($length)
{
  $out = "";
  while($length-- > 0)
    $out .= chr(rand(0,255));
  return $out;
}

?>
