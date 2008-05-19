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

  echo "\nProcessing account '" . $account . "'...\n";
  chdir($maildir . '/' . $account);
  process_account(".");
  chdir("../..");
}

exit();

// Makes the given subdirectory and all its subdirectories maildir-compatible
function process_account($dir)
{
  // Process files and directories below given directory
  $entries = scandir($dir);
  while(count($entries) > 0) {
    $entry = array_shift($entries);

    // Skip current and parent directory
    if($entry == "." || $entry == ".." || $entry == "cur" || $entry == "new" || $entry == "tmp")
      continue;

		if(is_subdir($entry))	{
			// .inbox.blaat.henk > .inbox.directory/.blaat.directory/henk
		  $newname = kmail_subdirname($entry);
	 	  if(DEBUG) {
        echo "\tCreating dir '" . $newname . "' and renaming '" . $entry . "' to it.\n";
			} else {
				$stat = stat($entry);
				$mode = $stat["mode"] & 0777;
				mkdir($newname, $mode, true);
				rename($entry, $newname);
			}
		}
		else {
			// .inbox > inbox
			$newname = substr($entry, 1);
		  if(DEBUG)
        echo "\tRenaming dir '". $entry . "' to '" . $newname . "'.\n";
      else
				rename($entry, $newname);
		}
	}
}

// Give the kmail-subdirname for a given courier-subdirname
function kmail_subdirname($dir)
{
	// .inbox.blaat.henk > henk
	$lastpart = substr($dir, strrpos($dir, ".") + 1);

	// .inbox.blaat.henk > inbox.blaat
	$firstpart = substr($dir, 1, strrpos($dir, ".") - 1);

	// .inbox.directory/.blaat.directory/henk
	$newname = "." . str_replace(".", ".directory/.", $firstpart) . ".directory/" . $lastpart;
	
	return $newname;
}

function is_subdir($dir)
{
	return substr_count($dir, '.') > 1;
}

?>
