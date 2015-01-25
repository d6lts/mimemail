<?php

/**
 * @file
 * The theme system, which controls the output of the messages.
 */

function mimemail_theme_theme() {
  return array(
    'mimemail_message' => array(
      'arguments' => array('subject' => NULL, 'body' => NULL, 'mailkey' => NULL, 'recipient' => NULL),
      'template' => 'mimemail-message',
      'pattern' => 'mimemail_message__',
      'file' => 'mimemail.theme.inc',
      'path' => drupal_get_path('module', 'mimemail') .'/theme',
    )
  );
}

/**
 * A preprocess function for theme('mimemail_message').
 *
 * The $variables array initially contains the following arguments:
 * - $subject: The message subject.
 * - $body: The message body in HTML format.
 * - $mailkey: The mailkey associated with the message.
 * - $recipient: An email address or user object who is receiving the message.
 */
function template_preprocess_mimemail_message(&$variables) {
  $theme = variable_get('theme_default', NULL);

  // Fetch the theme for the current domain.
  if (module_exists('domain_theme')) {
    // Assign the selected theme, based on the active domain.
    global $_domain;
    $domain_theme = domain_theme_lookup($_domain['domain_id']);
    // The above returns -1 on failure.
    if ($domain_theme != -1) {
      $theme = $domain_theme['theme'];
    }
  }

  $themepath = (function_exists('path_to_subtheme')) ? path_to_subtheme() : drupal_get_path('theme', $theme);

  $sitestyle = variable_get('mimemail_sitestyle', 1);
  $mailstyles = file_scan_directory($themepath, '^mail\.css*$');

  $css_all = drupal_add_css();
  $css_files_mail = $css_files_screen = array();
  foreach ($css_all as $media => $types) {
    foreach ($types as $type => $file) {
      // Gather and always include CSS files with 'email' media.
      if ($media == 'email') {
        $css_files_mail[$media][$type] = $file;
      }
      // Gather and include other site style sheets if enabled.
      elseif ($type == 'theme' && ($media == 'all' || $media == 'screen')) {
        $css_files_screen[$media][$type] = $file;
      }
    }
  }
  $styles = _mimemail_css_file_paths($css_files_mail);

  // Check recursively for the existence of a mail.css file in the default theme folder.
  if (!empty($mailstyles)) {
    foreach ($mailstyles as $mailstyle) {
      array_push($styles, $mailstyle->filename);
    }
  }
  // When no mail.css was found, include site style sheets if enabled.
  elseif ($sitestyle) {
    // Grab local.css if it exists (support for Fusion based themes).
    $local = $themepath . '/css/local.css';
    if (@file_exists($local)) {
      $css_files_screen['all']['theme'][$local] = TRUE;
    }
    $styles = array_merge($styles, _mimemail_css_file_paths($css_files_screen));
  }

  // Process each style sheet.
  $css = '';
  foreach ($styles as $style) {
    if (!empty($style) && @file_exists($style)) {
      $css .= drupal_load_stylesheet($style, TRUE);
    }
  }

  // Wordwrap to adhere to RFC821.
  $css = wordwrap($css, 700);
  $variables['css'] = $css;

   // Process mailkey to be a proper CSS class.
  $variables['mailkey'] = 'mail-'. str_replace('_', '-', $variables['mailkey']);
}

/**
 * Convert an array of CSS files to an array of file paths.
 */
function _mimemail_css_file_paths($css) {
  return explode("\n", preg_replace('|<link.*href="(' . $GLOBALS['base_url'] . ')?' . base_path()
                        . '([^"?]*)[?"].*|', '\2', drupal_get_css($css)));
}
