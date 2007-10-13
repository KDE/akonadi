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

// Process commandline arguments
$args = process_arguments($argv);
$attachment_chance = $args["chance"];
$attachment_minsize = $args["minsize"];
$attachment_maxsize = $args["maxsize"];

// Location of maildirs
$maildirname = "maildir";

if(($maildir = opendir($maildirname)) === FALSE)
  die("Error opening " . $maildirname . "\n");

$c = 0;
$a = 0;

// Process all mail accounts
while($account = readdir($maildir))
{
  // Skip current and parent directory
  if($account == "." || $account == "..")
    continue;

  echo "Processing account '" . $account . "'...\n";

  // Process all subdirectories
  $subdirs = scandir($maildirname . "/" . $account);
  foreach($subdirs as $subdir)
  {
    // Skip current and parent directory
    if($subdir == "." || $subdir == ".." || $subdir == "cur" || $subdir == "new" || $subdir == "tmp")
      continue;

    echo " Processing directory '" . $subdir . "'...\n";

    // Generate random attachment data
    $attachment_data = substr(base64_encode(random_string($attachment_maxsize)), 0, $attachment_maxsize);

    $maillocation = $maildirname . "/" . $account . "/" . $subdir . "/cur";
    if($dirhandle = @opendir($maillocation))
    {
      // Read next mail file
      while($mailfile = readdir($dirhandle))
      {
        $c++;
        // DEBUG: if($c == 4) exit();

        // Skip current and parent directory
        if($mailfile == "." || $mailfile == "..")
          continue;

        // Add attachment with a certain chance
        if(rand(0, 100) < $attachment_chance)
        {
          $a++;

          // Read mail file into array
          if(($mail = file($maillocation . "/" . $mailfile)) !== FALSE)
          {
            // Open output file
            $fd = fopen($maillocation . "/" . $mailfile, "w");
            // DEBUG: $fd = fopen("php://stdout", "w");

            if($fd)
            {
              // Generate MIME content boundary
              $boundary = random_text(50);

              // Process original mail file, line by line
              $indata = false;
              foreach($mail as $line)
              {
                $done = false;

                // Find contenttype header
                if(strpos($line, "Content-Type:") === 0)
                {
                  // Save original contenttype header
                  $original_contenttype = $line;

                  // Replace by new one
                  fwrite($fd, "Content-Type: multipart/mixed; boundary=\"" . $boundary . "\"\n");
                  $done = true;
                }

                // At start of data, insert boundary and original contenttype header
                if(!$indata && ($line == "\n" || $line == "\r\n" || $line == ""))
                {
                  fwrite($fd, "\nThis is a multi-part message in MIME format.\n\n" .
                              "--" . $boundary . "\n" .
                              $original_contenttype . "\n");
                  $indata = true;
                }

                // If this line isn't processed yet, just write it back to the file
                if(!$done || $indata)
                  fwrite($fd, $line);
              }

              // Add boundary, new content headers and random attachment data
              $attachment_filename = random_text(10) . ".bin";
              fwrite($fd, "\n--" . $boundary . "\n" .
                          "Content-Type: application/octet-stream;\n\tname=\"" . $attachment_filename . "\"\n" .
                          "Content-Transfer-Encoding: base64\n" .
                          "Content-Description: " . $attachment_filename . "\n" .
                          "Content-Disposition: attachment;\n\tfilename=\"" . $attachment_filename . "\"\n\n");

              // Add random attachment data
              for($i = rand($attachment_minsize, $attachment_maxsize); $i > 0;)
              {
                fwrite($fd, substr($attachment_data, $attachment_maxsize-$i, 76) . "\n");
                $i -= 77;
              }

              fwrite($fd, "\n--" . $boundary . "--\n");

              fclose($fd);
            }
          }
        }
      }
    }
  }
}

echo "Number of emails processed: " . $c . "\n";
echo "Number of attachments added: " . $a . "\n";
closedir($maildir);
exit();

function random_string($length)
{
  $out = "";
  while($length-- > 0)
    $out .= chr(rand(0,255));
  return $out;
}

function random_text($length)
{
  $out = "";
  while($length-- > 0)
  {
    $n = rand(0, 61);
    if($n < 10)
      $c = chr($n+48);
    else if($n < 36)
      $c = chr($n-10+65);
    else
      $c = chr($n-36+97);

    $out .= $c;
  }
  return $out;
}

function process_arguments($argv)
{
  $_ARG = array();
  if(is_array($argv))
  {
    foreach ($argv as $arg)
    {
      if (ereg('--[a-zA-Z0-9]*=.*',$arg))
      {
        $str = split("=",$arg); $arg = '';
        $key = ereg_replace("--",'',$str[0]);
        for ( $i = 1; $i < count($str); $i++ )
        {
          $arg .= $str[$i];
        }
        $_ARG[$key] = $arg;
      }
      elseif(ereg('-[a-zA-Z0-9]',$arg))
      {
        $arg = ereg_replace("-",'',$arg);
        $_ARG[$arg] = true;
      }
    }
  }

  return $_ARG;
}

?>
