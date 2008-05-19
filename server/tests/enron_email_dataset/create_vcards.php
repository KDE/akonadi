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

// Location of input files
$vcarddir = "vcards";

// Process all accounts
$inputfiles = scandir($vcarddir);
foreach($inputfiles as $inputfile)
{
  // Skip current and parent directory
  if($inputfile == "." || $inputfile == "..")
    continue;

  $account = substr($inputfile, 0, strlen($inputfile) - 4);
  $input = fopen($vcarddir . "/" . $inputfile, "r");
  if(!DEBUG)
    $output = fopen($vcarddir . "/" . $account . ".vcf", "w");

  echo "\tProcessing account " . $account . "...\n";
  $c = 0;

  if($input && $output)
  {
    // First line consists of headers, we don't need it
    fgets($input);

    // Process all contacts
    while($line = fgets($input))
    {
      $c++;

      $contact = explode(";", $line);

      // Create a vCard from the account information, add fake extra information
      $vcard = "BEGIN:VCARD\n";

      // ADR, N, URL

      // 25% chance of a random birthday
      if(rand(0,3) < 1)
        $vcard .= "BDAY:" . rand(date("Y")-60,date("Y")-20) . "-" . rand(1,12) . "-" . rand(1,28) . "T00:00:00Z". "\n";

      $vcard .= "COMPANY:" . $contact[1] . "\n";
      $vcard .= "EMAIL:" . $contact[2] . "";
      $vcard .= "FN:" . $contact[0] . "\n";

      // 2% chance of a fake photograph
      if(rand(0,50) < 1)
        $vcard .= "PHOTO;ENCODING=b;TYPE=application/octet-stream:" . base64_encode(random_string(10000,20000)) . "\n";

      // 75% chance of a random telephone number
      if(rand(0,3) < 3)
        $vcard .= "TEL;TYPE=" . (rand(0,1) == 0 ? "HOME" : "CELL") . ":0" . random_number(2) . (rand(0,1)==1 ? "-" : "") . random_number(7) . "\n";

      $vcard .= "UID:" . random_uid() . "\n";

      // 25% chance of a URL based on the contact's email address
      if(rand(0,3) < 1)
        $vcard .= "URL:" . remove_quotes(random_url($contact[2])) . "\n";

      $vcard .=
        "VERSION:3.0\n".
        "END:VCARD\n\n";

      if(DEBUG)
        echo("\t\t" . $output);
      else
        fputs($output, $vcard);

      /* Sample vCard, for reference:
      BEGIN:VCARD
      ADR;TYPE=home:;;Straat 1;Plaats;Provincie;Postcode;Netherlands
      BDAY:0000-00-00T00:00:00Z
      EMAIL:address
      FN:Full Name
      N:Last;First;tussenvoegsels;;
      TEL;TYPE=HOME:0123456789
      UID:random geneuzel
      URL:http://www.blabla.bl
      VERSION:3.0
      END:VCARD
      */
    }
  }

  echo "\tNumber of vcards created: " . $c . "\n";
  unlink($vcarddir . "/" . $inputfile);
}

exit();

function random_number($length)
{
  $out = "";
  while($length-- > 0)
    $out .= rand(0,9);
  return $out;
}

function random_uid()
{
  $out = "";
  $length = 10;
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

function random_url($email)
{
  $out = "http://" . (rand(0,5) < 4 ? "www." : "");
  $out .= substr($email, strrpos($email, "@") + 1);

  return $out;
}

function random_string($length)
{
  $out = "";
  while($length-- > 0)
    $out .= chr(rand(0,255));
  return $out;
}

function remove_quotes($str)
{
  $chars = "\"' \t\n\r\0";
  return ltrim(rtrim($str, $chars.">"), $chars."<");
}

?>