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

// Location of maildirs
$maildir = "maildir";

if(($dirhandle = opendir($maildir)) === FALSE)
  die("Error opening " . $maildir . "\n");

// Process all mail accounts
while($account = readdir($dirhandle)) {
  // Skip current and parent directory
  if($account == "." || $account == "..")
    continue;

  echo "Processing account '" . $account . "'...\n";
  chdir($maildir . '/' . $account);
  process_subdir(".");
  chdir("../..");
}

exit();

// Makes the given subdirectory and all its subdirectories maildir-compatible
function process_subdir($dir)
{
  // Create dirs 'new'/'cur'/'tmp' with same mode as given directory
  $stat = stat($dir);
  $mode = $stat["mode"] & 0777;

  if(DEBUG)
    echo "Creating " . $dir . "/{new/cur/tmp}.\n";
  else {
    mkdir($dir . "/new", $mode);
    mkdir($dir . "/cur", $mode);
    mkdir($dir . "/tmp", $mode);
  }

  // Process files and directories below given directory
  $entries = scandir($dir);
  while(count($entries) > 0) {
    $entry = array_shift($entries);
    
     // Skip current and parent directory
    if($entry == "." || $entry == ".." || $entry == "cur" || $entry == "new" || $entry == "tmp")
      continue;

    if($dir == ".")
      $fullname = $entry;
    else
      $fullname = $dir . '/' . $entry;

    if(is_dir($fullname)) {
      // Process subdirectories recursively
      process_subdir($fullname);

      // Rename subdir to maildir-compatible name
      $newname = "." . str_replace('/', '.', $fullname);

      if(DEBUG)
        echo "Renaming dir ". $fullname . " to " . $newname . ".\n";
      else
        rename($fullname, $newname);
    }
    else if(is_file($fullname)) {
      // Move file to 'cur' subdirectory
      if(DEBUG)
        echo "Renaming file " .  $fullname ." to " . $dir . '/cur/' . $entry . ".\n";
      else
        rename($fullname, $dir . '/cur/' . $entry);
    }
  }
}

?>
