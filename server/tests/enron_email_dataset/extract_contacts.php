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

define("DEBUG", false);

// The header
$header = "Name;Company;Email\n";

// The mail headers to look for
$search_elements = array("X-To:", "To:", "X-From:", "From:"); //, "Cc:", "Bcc:");

// Location of maildirs
$maildirname = "maildir";

if(($maildir = opendir($maildirname)) === FALSE)
die("Error opening " . $maildirname . "\n");

$c = 0;

// Process all mail accounts
while($account = readdir($maildir))
{
  // Skip current and parent directory
  if($account == "." || $account == "..")
    continue;

  echo "Analyzing account '" . $account . "'...\n";

  $contacts = array();

  // Process all subdirectories
  $subdirs = scandir($maildirname . "/" . $account);
  foreach($subdirs as $subdir)
  {
    // Skip parent directory and maildir-specific subdirectories
    if($subdir == ".." || $subdir == "cur" || $subdir == "new" || $subdir == "tmp")
      continue;

    echo " Processing directory '" . $subdir . "'...\n";

    $maillocation = $maildirname . "/" . $account . "/" . $subdir . "/cur";
    if($dirhandle = @opendir($maillocation))
    {
      // Read next mail file
      while($mailfile = readdir($dirhandle))
      {
        $c++;

        // Open mail file
        $fd = fopen($maillocation . "/" . $mailfile, "r");
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
    }
  }

  // Write headers to output file
  if(!DEBUG) {
    $output = fopen("vcards/" . $account . ".txt", "w");
    fwrite($output, $header);
  } else
    echo($header);

  foreach($contacts as $contact)
  {
    $line = implode(";", $contact) . "\n";
    if($line != "\n")
    {
      if(DEBUG)
        echo($line);
      else
        fwrite($output, $line);
    }
  }

  echo " " . count($contacts) . " contacts found.\n";
}

echo "Number of messages analyzed: " . $c . "\n";
closedir($maildir);
exit();

// Try to fetch the company name from the name and/or email address
function interprete_company($name, $email)
{
  $return = "";

  $companies = array(
      "enron" => "Enron Corporation",
      "telusplanet" => "TELUS",
      "metronet" => "Metronet Rail",
      "arcfinancial" => "ARC Financial",
      "cadvision" => "CADVision",
      "bennettjones" => "Bennett Jones LLP",
      "home" => "@Home",
      "ca" => "Canadian Venture Capital Association",
      "cibc" => "CIBC Banking",
      "rpcl" => "Rakhit Petroleum Consulting Ltd.",
      "cn" => "Canadian National",
      "hotmail" => "MSN Hotmail",
      "tcel" => "Toronto Chinese for Ecological Living",
      "petersco", "Peters & Co Limited",
      "scotia-mcleod" => "Scotia McLeod",
      "globalsec" => "Global Securities Corp.",
      "abfg" => "Association of British Fungus Groups",
      "shl" => "SHL",
      "kineticres" => "Kinetic Research",
      "cdnoxy" => "CD Noxy"
    );

  // Check if name contains parentheses, use the part in between as Enron division
  if(eregi("^.*\(.*\)$", $name))
  {
    $first = strpos($name, "(");
    $second = strpos($name, ")", $first);
    $return = "Enron Corporation (" . substr($name, $first + 1, $second-$first-1) . ")";
  }
  else // Use the domain name of the email address as basis for company name
  {
    if($email != "")
    {
      $first = strpos($email, "@");
      $second = strpos($email, ".", $first);
      $return = substr($email, $first + 1, $second-$first-1);

      // Try to look up the full name of a company, otherwise return the raw name (with a capitalized first letter ;-))
      if(array_key_exists($return, $companies))
        $return = $companies[$return];
      else
        $return[0] = strtoupper($return[0]);
    }
  }

  return $return;
}

// Interprete the name and address information in the given pair
function interprete_values($one, $two)
{
  $return = array();

  if(is_composite($one))
  {
    // Interprete $one
    $return = interprete_composite($one);
  }
  elseif(is_composite($two))
  {
    // Interprete $two
    $return = interprete_composite($two);
  }
  elseif($one != "" && $two != "")
  {
    $one = remove_quotes($one);
    $two = remove_quotes($two);

    if(is_email_address($one))
      $return = array($two, "", $one);
    elseif(is_email_address($two))
      $return = array($one, "", $two);
  }

  $return[1] = interprete_company($return[0], $return[2]);

  return $return;
}

// Interprete a composite string, consisting of a name and an email address
function interprete_composite($string)
{
  $name = "";
  $address = "";

  for($i = 0; $i < strlen($string); $i++)
  {
    if($string[$i] == "<")
    {
      $name = substr($string, 0, $i-1);
      $address = substr($string, $i+1, strlen($string)-$i-2);
      break;
    }

  }

  return array(remove_quotes($name), "", remove_quotes($address));
}

// Split a string into substrings, but account for quotes
function explode_nicely($delimiters, $string)
{
  $results = array();
  $start_of_part = 0;
  $within_quotes = false;

  // Walk through the input string
  for($i = 0; $i < (strlen($string) - 1); $i++)
  {
    // Skip quoted parts of the input string
    if($string[$i] == "\"")
    {
      $within_quotes = !$within_quotes;
    }

    if(!$within_quotes)
    {
      // Check for occurrences of delimiter
      if(trim($string[$i], $delimiters) == "")
      {
        // Found: store substring in resultset
        $results[] = substr($string, $start_of_part, $i - $start_of_part);
        $i++;
        $start_of_part = $i;
      }
    }
  }

  return $results;

}

// Remove quotes and other useless characters from the beginning and end of a given string
function remove_quotes($str)
{
  $chars = "\"' \t\n\r\0";
  return ltrim(rtrim($str, $chars . ">"), $chars . "<");
}

// Check whether the given string is a valid email address
function is_email_address($str)
{
  return (eregi("^[_a-z0-9-]+(\.[_a-z0-9-]+)*@[a-z0-9-]+(\.[a-z0-9-]+)*(\.[a-z]{2,3})$", $str));
}

// Check whether the given string is a composite of a name and an email address
function is_composite($str)
{
  return (eregi("^.*<[_a-z0-9-]+(\.[_a-z0-9-]+)*@[a-z0-9-]+(\.[a-z0-9-]+)*(\.[a-z]{2,3})>$", $str));
}

?>
