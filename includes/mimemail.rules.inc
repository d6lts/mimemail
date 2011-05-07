<?php // $Id$

/**
 * @file
 * Rules actions for sending Mime-encoded emails.
 *
 * @addtogroup rules
 * @{
 */

/**
 * Implementation of hook_rules_action_info().
 */
function mimemail_rules_action_info() {
  return array(
    'mimemail_rules_action_mail_to_user' => array(
      'label' => t('Send an HTML mail to a user'),
      'arguments' => array(
        'user' => array('type' => 'user', 'label' => t('Recipient')),
      ),
      'module' => 'Mime Mail',
      'eval input' => array('from', 'to', 'cc', 'bcc', 'subject', 'message_html', 'message_plaintext', 'attachments'),
    ),
    'mimemail_rules_action_mail' => array(
      'label' => t('Send an HTML mail to an arbitrary mail address'),
      'module' => 'Mime Mail',
      'eval input' => array('from', 'to', 'cc', 'bcc', 'subject', 'message_html', 'message_plaintext', 'attachments'),
    ),
    'mimemail_rules_action_mail_to_users_of_role' => array(
      'label' => t('Send an HTML mail to all users of a role'),
      'module' => 'Mime Mail',
      'eval input' => array('from', 'to', 'cc', 'bcc', 'subject', 'message_html', 'message_plaintext', 'attachments'),
    ),
  );
}

/**
 * Action Implementation: Send a mail to a user and to an arbitrary mail address.
 */
function mimemail_rules_action_mail_to_user($user, $settings) {
  global $language;

  // Process settings.
  $settings = mimemail_rules_form_settings_process($settings);
  $from = $settings['from'];

  // If recipient field is empty send it to given user object.
  $to = empty($settings['to']) ? $user->mail : implode(',', $settings['to']);

  // Prepare the message but not send.
  $message = drupal_mail('mimemail', 'rules_action_mail', $to, $language, $settings, $from, FALSE);

  // Send the prepared message.
  $message = mimemail(
    $message['from'],
    $message['to'],
    $message['subject'],
    $message['body'],
    NULL ,
    $message['headers'],
    $message['params']['plaintext'],
    $message['params']['attachments'],
    $message['id']
  );

  if ($message['result']) {
    $recipients = array_merge(explode(',', $to), $settings['bcc'], $settings['cc']);
    watchdog('rules', 'HTML mail successfully sent to %recipient', array('%recipient' => implode(', ', $recipients)));
  }
}

/**
 * Action Implementation: rules_action_mail
 * This action makes use of the rules_action_mail_to_user action implementation.
 */
function mimemail_rules_action_mail($settings) {
  mimemail_rules_action_mail_to_user(NULL, $settings);
}

/**
 * Action: Send mail to all users of a specific role group(s).
 */
function mimemail_rules_action_mail_to_users_of_role($settings) {
  $recipients = array_filter(array_keys(array_filter($settings['recipients'])));

  // All authenticated users, which is everybody.
  if (in_array(DRUPAL_AUTHENTICATED_RID, $recipients)) {
    $result = db_query('SELECT mail FROM {users} WHERE uid > 0');
  }
  else {
    $rids = implode(',', $recipients);
    // Avoid sending emails to members of two or more target role groups.
    $result = db_query('SELECT DISTINCT u.mail FROM {users} u INNER JOIN {users_roles} r ON u.uid = r.uid WHERE r.rid IN ('. $rids .')');
  }

  // Process settings.
  $settings = mimemail_rules_form_settings_process($settings);

  // Now, actually send the mails.
  while (($account = db_fetch_object($result))) {
    $message = drupal_mail('mimemail', 'rules_action_role_mail',  $account->mail, user_preferred_language($account), array(), NULL, FALSE);
    $message = mimemail(
      $message['from'],
      $message['to'],
      $message['subject'],
      $message['body'],
      NULL ,
      $message['headers'],
      $message['params']['plaintext'],
      $message['params']['attachments'],
      $message['id']
    );
  }
  if ($message['result']) {
    $roles = array_intersect_key(user_roles(TRUE), drupal_map_assoc($recipients));
    watchdog('rules', 'HTML mail successfully sent to role(s) %roles.', array('%roles' => implode(', ', $roles)));
  }
}

/*
 * Helper function for processing form settings on execution time,
 * so that tokens and input evaluators apply.
 */
function mimemail_rules_form_settings_process($settings) {
  // We also handle CC and BCC if it's set.
  $settings['to'] = empty($settings['to']) ? array() : explode(',', $settings['to']);
  $settings['cc'] = empty($settings['cc']) ? array() : explode(',', $settings['cc']);
  $settings['bcc'] = empty($settings['bcc']) ? array() : explode(',', $settings['bcc']);

  foreach ($settings['to'] as $key => $address) {
    $settings['to'][$key] = str_replace(array("\r", "\n"), '', trim($address));
  }
  foreach ($settings['cc'] as $key => $address) {
    $settings['cc'][$key] = str_replace(array("\r", "\n"), '', trim($address));
  }
  foreach ($settings['bcc'] as $key => $address) {
    $settings['bcc'][$key] = str_replace(array("\r", "\n"), '', trim($address));
  }

  $settings['from'] = str_replace(array("\r", "\n"), '', $settings['from']);

  return $settings;
}

/**
 * @}
 */

