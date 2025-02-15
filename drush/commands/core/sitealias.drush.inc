<?php

/**
 * @file
 *   Site alias commands. @see example.drushrc.php for details.
 */

function sitealias_drush_help($section) {
  switch ($section) {
    case 'drush:site-alias':
      return dt('Print an alias record.');
  }
}

function sitealias_drush_command() {
  $items = array();

  $items['site-alias'] = array(
    'callback' => 'drush_sitealias_print',
    'description' => 'Print site alias records for all known site aliases and local sites.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'arguments' => array(
      'site' => 'Site specification alias to print',
    ),
    'options' => array(
      'table' => 'Display the alias name, root, uri and host in a table.',
      'full' => 'Print the full alias record for each site.  Default when aliases are specified on the command line.',
      'component' => 'Print only the specified element from the full alias record.',
      'short' => 'Print only the site alias name.  Default when no command line arguments are specified.',
      'pipe' => 'Print the long-form site specification for each site.',
      'with-db' => 'Include the databases structure in the full alias record.',
      'with-db-url' => 'Include the short-form db-url in the full alias record.',
      'no-db' => 'Do not include the database record in the full alias record (default).',
      'with-optional' => 'Include optional default items.',
      'alias-name' => 'For a single alias, set the name to use in the output.',
      'local' => 'Only display sites that are available on the local system (remote-site not set, and Drupal root exists).',
    ),
    'aliases' => array('sa'),
    'examples' => array(
      'drush site-alias' => 'List all alias records known to drush.',
      'drush site-alias @dev' => 'Print an alias record for the alias \'dev\'.',
    ),
    'topics' => array('docs-aliases'),
  );
  return $items;
}

/**
 * Command argument complete callback.
 *
 * @return
 *  Array of available site aliases.
 */
function sitealias_sitealias_print_complete() {
  $site_specs = array_keys(_drush_sitealias_all_list());
  ksort($site_specs);
  return array('values' => $site_specs);
}

/**
 * Return a list of all site aliases known to drush.
 *
 * The array key is the site alias name, and the array value
 * is the site specification for the given alias.
 */
function _drush_sitealias_alias_list() {
  return drush_get_context('site-aliases');
}

/**
 * Return a list of all of the local sites at the current drupal root.
 *
 * The array key is the site folder name, and the array value
 * is the site specification for that site.
 */
function _drush_sitealias_site_list() {
  $site_list = array();
  $base_path = drush_get_context('DRUSH_DRUPAL_ROOT') . '/sites';
  $files = drush_scan_directory($base_path, '/settings\.php/', array('.', '..', 'CVS', 'all'));
  foreach ($files as $filename => $info) {
    if ($info->basename == 'settings.php') {
      $alias_record = drush_sitealias_build_record_from_settings_file($filename);
      if (!empty($alias_record)) {
        $site_list[drush_sitealias_uri_to_site_dir($alias_record['uri'])] = $alias_record;
      }
    }
  }
  return $site_list;
}

/**
 * Return the list of all site aliases and all local sites.
 */
function _drush_sitealias_all_list() {
  drush_sitealias_load_all();
  return array_merge(_drush_sitealias_alias_list(), _drush_sitealias_site_list());
}

/**
 * Return the list of sites (aliases or local) that the
 * user specified on the command line.  If none were specified,
 * then all are returned.
 */
function _drush_sitealias_user_specified_list() {
  $command = drush_get_command();
  $specifications = $command['arguments'];
  $site_list = array();

  // Did the user specify --short or --full output?
  $specified_output_style = drush_get_option(array('full', 'short'), FALSE);

  // Iterate over the arguments and convert them to alias records
  if (!empty($specifications)) {
    $site_list = drush_sitealias_resolve_sitespecs($specifications);
    if (!$specified_output_style) {
      drush_set_option('full', TRUE);
    }
  }
  // If the user provided no args, then we will return everything.
  else {
    $site_list = _drush_sitealias_all_list();

    // Filter out the hidden items
    foreach ($site_list as $site_name => $one_site) {
      if (array_key_exists('#hidden', $one_site)) {
        unset($site_list[$site_name]);
      }
    }
  }

  // Filter out the local sites
  if (drush_get_option('local', FALSE)) {
    foreach ($site_list as $site_name => $one_site) {
      if ( (array_key_exists('remote-site', $one_site)) ||
           (!array_key_exists('root', $one_site)) ||
           (!is_dir($one_site['root']))
         ) {
        unset($site_list[$site_name]);
      }
    }
  }
  return $site_list;
}

/**
 * Print out the specified site aliases using the format
 * specified.
 */
function drush_sitealias_print() {
  // Call bootstrap max, unless the caller requested short output
  if (!drush_get_option('short', FALSE)) {
    drush_bootstrap_max();
  }

  $site_list = _drush_sitealias_user_specified_list();
  ksort($site_list);
  $table_output = drush_get_option('table');
  $full_output = drush_get_option('full');
  $long_output = drush_get_option('long');
  $with_db = (drush_get_option('with-db') != null) || (drush_get_option('with-db-url') != null);

  $site_specs = array();
  $rows = array();
  foreach ($site_list as $site => $alias_record) {
    if (!array_key_exists('site-list', $alias_record)) {
      $site_specs[] = drush_sitealias_alias_record_to_spec($alias_record, $with_db);
    }
    if (isset($table_output)) {
      $alias_record += array('root' => '', 'uri' => '', 'remote-host' => '');
      $row = array($site, $alias_record['root'], $alias_record['uri'], $alias_record['remote-host']);
      if (empty($rows)) {
        $header = array(dt('Name'), dt('Root'), dt('URI'), dt('Host'));
        $rows = array($header);
      }
      $rows[] = $row;
    }
    elseif (isset($full_output)) {
      $component = drush_get_option('component');
      if ($component) {
        if (array_key_exists($component, $alias_record)) {
          drush_print($alias_record[$component]);
        }
        else {
          drush_set_error('DRUSH_NO_SUCH_ELEMENT', dt('The element @component was not found in the alias record for @site.', array('@component' => $component, '@site' => $site)));
        }
      }
      else {
        _drush_sitealias_print_record($alias_record, $site);
      }
    }
    else {
      drush_print($site);
    }
  }
  $site_specs = array_unique($site_specs);
  asort($site_specs);
  drush_print_pipe(array_unique($site_specs));
  if (!empty($rows)) {
    drush_print_table($rows, TRUE);
  }
}

/**
 * Given a site alias name, print out a php-syntax
 * representation of it.
 *
 * @param alias_record
 *   The name of the site alias to print
 */
function _drush_sitealias_print_record($alias_record, $site_alias = '') {
  $output_db = drush_get_option('with-db');
  $output_db_url = drush_get_option('with-db-url');
  $output_optional_items = drush_get_option('with-optional');

  // Make sure that the default items have been added for all aliases
  _drush_sitealias_add_static_defaults($alias_record);

  // Include the optional items, if requested
  if ($output_optional_items) {
    _drush_sitealias_add_transient_defaults($alias_record);
  }

  drush_sitealias_resolve_path_references($alias_record);

  if (isset($output_db_url)) {
    drush_sitealias_add_db_url($alias_record);
  }
  if (isset($output_db_url) || isset($output_db)) {
    drush_sitealias_add_db_settings($alias_record);
  }
  // If the user specified --with-db-url, then leave the
  // 'db-url' entry in the alias record (unless it is not
  // set, in which case we will leave the 'databases' record instead).
  if (isset($output_db_url)) {
    if (isset($alias_record['db-url'])) {
      unset($alias_record['databases']);
    }
  }
  // If the user specified --with-db, then leave the
  // 'databases' entry in the alias record.
  else if (isset($output_db)) {
    unset($alias_record['db-url']);
  }
  // If neither --with-db nor --with-db-url were specified,
  // then remove both the 'db-url' and the 'databases' entries.
  else {
    unset($alias_record['db-url']);
    unset($alias_record['databases']);
  }

  // The alias name will be the same as the site alias name,
  // unless the user specified some other name on the command line.
  $alias_name = drush_get_option('alias-name');
  if (!isset($alias_name)) {
    $alias_name = $site_alias;
    if (empty($alias_name) || is_numeric($alias_name)) {
      $alias_name = drush_sitealias_uri_to_site_dir($alias_record['uri']);
    }
  }

  // We don't want the name to go into the output
  unset($alias_record['#name']);
  unset($alias_record['#hidden']);

  // We only want to output the 'root' item; don't output the '%root' path alias
  if (array_key_exists('path-aliases', $alias_record) && array_key_exists('%root', $alias_record['path-aliases'])) {
    unset($alias_record['path-aliases']['%root']);
    // If there is nothing left in path-aliases, then clear it out
    if (count($alias_record['path-aliases']) == 0) {
      unset($alias_record['path-aliases']);
    }
  }

  // Alias names contain an '@' when referenced, but do
  // not contain an '@' when defined.
  if (substr($alias_name,0,1) == '@') {
    $alias_name = substr($alias_name,1);
  }

  if (!drush_get_option('show-passwords', FALSE)) {
    drush_unset_recursive($alias_record, 'password');
  }

  $exported_alias = var_export($alias_record, TRUE);
  drush_print('$aliases[\'' . $alias_name . '\'] = ' . $exported_alias . ';');
}

/**
 * Use heuristics to attempt to convert from a site directory to a URI.
 * This function should only be used when the URI really is unknown, as
 * the mapping is not perfect.
 *
 * @param site_dir
 *   A directory, such as domain.com.8080.drupal
 *
 * @return string
 *   A uri, such as http://domain.com:8080/drupal
 */
function _drush_sitealias_site_dir_to_uri($site_dir) {
  // Protect IP addresses NN.NN.NN.NN by converting them
  // temporarily to NN_NN_NN_NN for now.
  $uri = preg_replace("/([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)/", "$1_$2_$3_$4", $site_dir);
  // Convert .[0-9]+. into :[0-9]+/
  $uri = preg_replace("/\.([0-9]+)\./", ":$1/", $uri);
  // Convert .[0-9]$ into :[0-9]
  $uri = preg_replace("/\.([0-9]+)$/", ":$1", $uri);
  // Convert .(com|net|org|info). into .(com|net|org|info)/
  $uri = str_replace(array('.com.', '.net.', '.org.', '.info.'), array('.com/', '.net/', '.org/', '.info/'), $uri);

  // If there is a / then convert every . after the / to /
  // Then again, if we did this we would break if the path contained a "."
  // I hope that the path would never contain a "."...
  $pos = strpos($uri, '/');
  if ($pos !== false) {
    $uri = substr($uri, 0, $pos + 1) . str_replace('.', '/', substr($uri, $pos + 1));
  }

  // n.b. this heuristic works all the time if there is a port,
  // it also works all the time if there is a port and no path,
  // but it does not work for domains such as .co.jp with no path,
  // and it can fail horribly if someone makes a domain like "info.org".
  // Still, I think this is the best we can do short of consulting DNS.

  // Convert from NN_NN_NN_NN back to NN.NN.NN.NN
  $uri = preg_replace("/([0-9]+)_([0-9]+)_([0-9]+)_([0-9]+)/", "$1.$2.$3.$4", $site_dir);

  return 'http://' . $uri;
}
