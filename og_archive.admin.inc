<?php

/**
 * @file
 * Page call back for admin settings of OG archive module.
 */

/**
 * Configuration form for admin settings of OG archive.
 * @return $form
 */
function og_archive_settings() {
  $defaults = og_archive_get_settings();
  $form['roles_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Archive/Unarchive Roles'),
    '#description' => t('Settings for roles who can archive and unarchive groups.'),
    '#collapsible' => FALSE
  );
  $form['roles_fieldset']['roles_archive'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Roles who can archive and unarchive groups'),
    '#description' => t('Select the list of roles who can archive and unarchive groups. </br>Users with the selected roles will have full access to archived group nodes.'),
    '#options' => user_roles(TRUE),
    '#default_value' => $defaults['roles_archive']
  );
  $form['node_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Node settings'),
    '#collapsible' => FALSE
  );
  $form['node_fieldset']['node_readonly'] = array(
    '#type' => 'radios',
    '#title' => t(' Group Access'),
    '#description' => t('Access type for an archived group. Archived groups are not editable by roles not having archiving permission.'),
    '#options' => array('Deny', 'Readonly', 'Unpublish'),
    '#default_value' => $defaults['archive_group_nodes']
  );
  $form['node_fieldset']['node_associated_archive'] = array(
    '#type' => 'radios',
    '#title' => t('Associated Group Node Access'),
    '#description' => t('Access type for all associated nodes of an archived group. Archived groups related nodes are not editable by roles not having archiving permission.</br>Note: Selected node access configuration is overridden if all parent groups are not archived.'),
    '#default_value' => $defaults['archive_associated_nodes'],
    '#options' => array('Deny', 'Readonly', 'Unpublish'),
  );
  $form['node_fieldset']['archive_associated_nodes_creation'] = array(
    '#type' => 'radios',
    '#title' => t('Associated Group Node Creation'),
    '#description' => t('Users can create nodes in archived groups.'),
    '#default_value' => $defaults['archive_associated_nodes_creation'],
    '#options' => array('Deny', 'Allow'),
  );
  $form['node_fieldset']['node_editusers'] = array(
    '#type' => 'radios',
    '#title' => t(' Manage Group Users'),
    '#description' => t('Users can be added or removed to archived groups.'),
    '#default_value' => $defaults['archive_edit_users'],
    '#options' => array('Deny', 'Allow'),
  );
  $form['archive_group'] = array(
    '#type' => 'submit',
    '#value' => 'Save'
  );
  $form['#submit'] = array('og_archive_settings_submit');
  return $form;
}

/*
 * Submit function for OG archive settings form.
 */
function og_archive_settings_submit($form, &$form_state) {
  $roles_archive = array_values($form['roles_fieldset']['roles_archive']['#value']);
  $variable = array( 'roles_archive' => $roles_archive, 'archive_group_nodes' => $form_state['values']['node_readonly'], 'archive_associated_nodes' => $form_state['values']['node_associated_archive'], 'archive_associated_nodes_creation' => $form_state['values']['archive_associated_nodes_creation'], 'archive_edit_users' => $form_state['values']['node_editusers'] );
  $setting = og_archive_get_settings();
  // If setting has changed.
  if ($setting['archive_group_nodes'] != $variable['archive_group_nodes']) {
    // Publish unpublish groups.
    if ($variable['archive_group_nodes'] == 2)
      og_archive_publishing( 'unpublish', 'group' );
    elseif ($setting['archive_group_nodes'] == 2)
      og_archive_publishing( 'publish', 'group' );
  }
  if ($setting['archive_associated_nodes'] != $variable['archive_associated_nodes']) {
    // Publish unpublish nodes.
    if ($variable['archive_associated_nodes'] == 2)
      og_archive_publishing( 'unpublish', 'group post' );
    elseif ($setting['archive_associated_nodes'] == 2)
      og_archive_publishing( 'publish', 'group post' );
  }
  $flag = flag_get_flag( "archive" );
  $flag->roles['flag'] = $roles_archive;
  $flag->roles['unflag'] = $roles_archive;
  $flag->save();
  variable_set( 'og_archive_settings', serialize( $variable ) );
  drupal_set_message( "The configuration has been saved." );
}

/*
 * Publish and unpublish all nodes or groups
 */
function og_archive_publishing($op, $type) {
  $query = db_query( "SELECT content_id from {flag_content} where fid=(SELECT fid FROM {flags} where name='archive' and content_type='node')" );
  $count = 0;
  while ( $r = db_fetch_array( $query ) ) {
    if ($type == 'group') {
      $count += 1;
      //drupal_set_message($r['content_id']);
      og_archive_pub_unpub( $op, $r['content_id'] );
    }
    else {
      $archive_list = og_archive_publish_list( $r['content_id'] );
      //print_r($archive_list);exit;
      foreach ( $archive_list as $k => $nid ) {
        $count += 1;
        og_archive_pub_unpub( $op, $nid );
      }
    }
  }
  $op = ($op == 'publish') ? 'republish' : $op;
  drupal_set_message(format_plural($count, ucwords($op) . " $count $type node", ucwords($op) . " @count $type nodes"));
}
