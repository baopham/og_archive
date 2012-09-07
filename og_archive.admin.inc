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
  $defaults = variable_get('og_archive_settings', array());
  $flag = flag_get_flag("archive");
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
    '#default_value' => $flag->roles['flag']
  );
  $form['node_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Node settings'),
    '#collapsible' => FALSE
  );
  $form['node_fieldset']['node_readonly'] = array(
    '#type' => 'radios',
    '#title' => t('Group Access'),
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
    '#title' => t('Manage Group Users'),
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
  $roles_archive = array();
  foreach ($form_state['values']['roles_archive'] as $k => $v) {
    if ($v)
      $roles_archive[] = $v;
  }
  $variable = array('roles_archive' => $roles_archive, 'archive_group_nodes' => $form_state['values']['node_readonly'], 'archive_associated_nodes' => $form_state['values']['node_associated_archive'], 'archive_associated_nodes_creation' => $form_state['values']['archive_associated_nodes_creation'], 'archive_edit_users' => $form_state['values']['node_editusers']);
  $setting = variable_get('og_archive_settings', array());
  // If setting has changed.
  if ($setting['archive_group_nodes'] != $variable['archive_group_nodes']) {
    // Publish unpublished groups.
    $archive_grp = ($form_state['values']['node_readonly'] == 2) ? 'unpublish' : 'publish';
    og_archive_publishing($archive_grp, 'group');
  }
  if ($setting['archive_associated_nodes'] != $variable['archive_associated_nodes']) {
    // Publish unpublished nodes.
    $archive_node = ($form_state['values']['node_associated_archive'] == 2) ? 'unpublish' : 'publish';
    og_archive_publishing($archive_node, 'group content');
  }
  $flag = flag_get_flag("archive");
  $flag->roles['flag'] = $roles_archive;
  $flag->save();
  variable_set('og_archive_settings', $variable);
  drupal_set_message(t("The configuration has been saved."));
}

/*
 * Publish and unpublish all nodes or groups
 *
 * @param $op
 *   A string: 'publish' or 'unpublish'
 * @param $type
 *   A string: 'group' or 'group content'
 */
function og_archive_publishing($op, $type) {
  $query = db_select('flagging', 'fn');
  $query->join('flag', 'f', "fn.fid = f.fid AND f.name = 'archive' AND f.entity_type = 'node'");
  $query->fields('fn', array('entity_id'));
  $result = $query->execute();

  $count = 0;
  foreach ($result as $record) {
    if ($type == 'group') {
      $count += 1;
      og_archive_pub_unpub($op, $record->entity_id);
    }
    else {
      $archive_list = og_archive_publish_list($record->entity_id);
      foreach ($archive_list as $k => $nid) {
        $count += 1;
        og_archive_pub_unpub($op, $nid);
      }
    }
  }
  $op = ($op == 'publish') ? 'republish' : $op;
  drupal_set_message(format_plural($count, ucwords($op) . " $count $type node", ucwords($op) . " @count $type nodes"));
}
