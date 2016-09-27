<?php

/**
 * @file
 *
 * Plugin to provide a eck_edit_form context
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t("Eck edit form"),
  'description' => t('A eck edit form.'),
  'context' => 'eck_pm_context_create_eck_edit_form',
  'edit form' => 'eck_pm_context_eck_edit_form_settings_form',
  'defaults' => array('id' => ''),
  'convert list' => array('entity_type' => t('Entity type'), 'type' => t('Bundle')),
  'convert' => 'eck_pm_context_eck_edit_convert',
  'placeholder form' => array(
    '#type' => 'textfield',
    '#description' => t('Enter the eck ID of a eck for this argument:'),
  ),
  'get child' => 'eck_pm_context_entity_edit_form_get_child',
  'get children' => 'eck_pm_context_entity_edit_form_get_children',
);

function eck_pm_context_entity_edit_form_get_child($plugin, $parent, $child) 
{
  $plugins = eck_pm_context_entity_edit_form_get_children($plugin, $parent);
  return $plugins[$parent . ':' . $child];
}

function eck_pm_context_entity_edit_form_get_children($plugin, $parent) 
{
  $entities = EntityType::loadAll();
  $plugins = array();
  foreach ($entities as $entity_type => $entityObj) {
    $entity = entity_get_info($entityObj->name);
    $child_plugin = $plugin;
    $child_plugin['title'] = $entity['label'] . ' edit form eck';
    $child_plugin['keyword'] = $entity_type;
    $child_plugin['context name'] = $entity_type . '_edit_form';
    $child_plugin['name'] = $parent . ':' . $entity_type;
    $child_plugin['description'] = t('Creates edit form for @entity', array('@entity' => $entity_type));
    $child_plugin_id = $parent . ':' . $entity_type;
    
    $plugins[$child_plugin_id] = $child_plugin;
  }
  
  return $plugins;
}

/**
 * It's important to remember that $conf is optional here, because contexts
 * are not always created from the UI.
 */
function eck_pm_context_create_eck_edit_form($empty, $eck = NULL, $conf = FALSE, $plugin) 
{
  static $creating = FALSE;
  $entity_type = $plugin['keyword'];
  $context = new ctools_context(array(
    'form', 'eck_edit', 'eck_form', 'eck_edit_form','eck_edit_form:'.$entity_type, 
    'eck', 'entity:'.$entity_type, $entity_type
  ));
  $context->plugin = 'eck_edit_form';

  if ($empty || ($creating)) {
    return $context;
  }
  $creating = TRUE;
  
  
  if ($conf) {
    // In this case, $eck is actually our $conf array.
    $id = is_array($eck) && isset($eck['id']) ? $eck['id'] : (is_object($eck) ? $eck->id : 0);

    if (module_exists('translation')) {
      if ($translation = module_invoke('translation', 'eck_id', $id, $GLOBALS['language']->language)) {
        $id = $translation;
        $reload = TRUE;
      }
    }

    if (is_array($eck) || !empty($reload)) {
      $eckList = entity_load($entity_type, array($id));
      $eck = array_shift($eckList);
    }
  }

  if (!empty($eck)) {
    $bundle_name = $eck->type;
    $form_id = "eck__entity__form_add_{$entity_type}_{$bundle_name}";

    $form_state = array('want form' => TRUE, 'build_info' => array('args' => array($eck)));

    $file = drupal_get_path('module', 'eck') . '/eck.entity.inc';
    require_once DRUPAL_ROOT . '/' . $file;
    // This piece of information can let other modules know that more files
    // need to be included if this form is loaded from cache:
    $form_state['build_info']['files'] = array($file);

    $form = drupal_build_form($form_id, $form_state);

    // Fill in the 'eck' portion of the context
    $context->data     = $eck;
    $context->title    = $bundle_name;
    $context->argument = isset($eck->id) ? $eck->id : $bundle_name;
    $context->entity_type = $entity_type;

    $context->form       = $form;
    $context->form_state = &$form_state;
    $context->form_id    = $form_id;
    $context->form_title = t('Submit @name', array('@name' => $bundle_name));
    $context->restrictions['type'] = array($bundle_name);
    $context->restrictions['form'] = array('form');

    $creating = FALSE;
    return $context;
  }
  $creating = FALSE;
}

function eck_pm_context_eck_edit_form_settings_form($form, &$form_state) 
{
  $conf = &$form_state['conf'];
  $entity_type = $conf['keyword'];
  $base_table = entity_get_info($entity_type)['base table'];

  $form['eck'] = array(
    '#title' => t('Enter the ID of a eck'),
    '#type' => 'textfield',
    '#maxlength' => 512,
    '#autocomplete_path' => "custom_eck_pm/autocomplete/{$entity_type}",
    '#weight' => -10,
  );

  if (!empty($conf['id'])) {
    $info = db_query('SELECT * FROM {'.$base_table.'} WHERE id = :id', array(':id' => $conf['id']))->fetchObject();
    if ($info) {
      $link = l(t("'%title' [eck id %id]", array('%title' => $info->title, '%id' => $info->id)), "eck/$info->id", array('attributes' => array('target' => '_blank', 'title' => t('Open in new window')), 'html' => TRUE));
      $form['eck']['#description'] = t('Currently set to !link', array('!link' => $link));
    }
  }

  $form['id'] = array(
    '#type' => 'value',
    '#value' => $conf['id'],
  );

  return $form;
}

/**
 * Validate a eck.
 */
function eck_pm_context_eck_edit_form_settings_form_validate($form, &$form_state) 
{
  $conf = $form_state['conf'];
  $entity_type = $conf['keyword'];
  $base_table = entity_get_info($entity_type)['base table'];
  
  // Validate the autocomplete
  if (empty($form_state['values']['id']) && empty($form_state['values']['eck'])) {
    form_error($form['eck'], t('You must select a eck.'));
    return;
  }

  if (empty($form_state['values']['eck'])) {
    return;
  }

  $id          = $form_state['values']['eck'];
  $preg_matches = array();
  $match        = preg_match('/\[id: (\d+)\]/', $id, $preg_matches);
  if (!$match) {
    $match = preg_match('/^id: (\d+)/', $id, $preg_matches);
  }

  if ($match) {
    $id = $preg_matches[1];
  }
  if (is_numeric($id)) {
    $eck = db_query('SELECT id FROM {'.$base_table.'} WHERE id = :id', array(':id' => $id))->fetchObject();
  }

  // Do not allow unpublished ecks to be selected by unprivileged users
  if (!$eck) {
    form_error($form['eck'], t('Invalid eck selected.'));
  }
  else {
    form_set_value($form['id'], $eck->id, $form_state);
  }
}

function eck_pm_context_eck_edit_form_settings_form_submit($form, &$form_state) 
{
  if ($form_state['values']['set_identifier']) {
    $eck = eck_load($form_state['values']['id']);
    $form_state['values']['identifier'] = $eck->id.' '.$eck->type;
  }

  // This will either be the value set previously or a value set by the
  // validator.
  $form_state['conf']['id'] = $form_state['values']['id'];
}

/**
 * Convert a context into a string.
 */
function eck_pm_context_eck_edit_convert($context, $type) 
{
  switch ($type) {
    case 'type':
      return $context->data->type;
  }
}

function eck_pm_context_entity_edit_form_get_bundles($entity_type)
{
    $entity = entity_get_info($entity_type);
    $entity_bundles = $entity['bundles'];
    
    foreach($entity_bundles as $id => $bundle){
        $bundles[$id] = $bundle['label'];
    }
    
    return $bundles;
}