<?php

/**
 * @file
 * Plugin to provide an relationship handler for term from eck.
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t('Eck edit form from eck'),
  'description' => t('Adds eck edit form from a eck context.'),
  'context' => 'eck_pm_eck_edit_form_from_eck_context',
  'get child' => 'eck_pm_eck_edit_form_from_eck_get_child',
  'get children' => 'eck_pm_eck_edit_form_from_eck_get_children',
);

function eck_pm_eck_edit_form_from_eck_get_child($plugin, $parent, $child) {
  $plugins = eck_pm_eck_edit_form_from_eck_get_children($plugin, $parent);
  return $plugins[$parent . ':' . $child];
}

function eck_pm_eck_edit_form_from_eck_get_children($plugin, $parent)
{
  $entities = EntityType::loadAll();
  $plugins = array();
  foreach ($entities as $entity_type => $entityObj) {
    $entity = entity_get_info($entityObj->name);
    $child_plugin = $plugin;
    $child_plugin['title'] = $entity['label'] . ' edit form from eck';
    $child_plugin['keyword'] = $entity_type;
    $child_plugin['context name'] = $entity_type . '_edit_form_from_eck';
    $child_plugin['name'] = $parent . ':' . $entity_type;
    $child_plugin['description'] = t('Adds eck edit form from a eck context for @entity', array('@entity' => $entity_type));
    $child_plugin['required context'] = new ctools_context_required(t('Eck @entity', array('@entity' => $entity_type)), $entity_type);
    $child_plugin_id = $parent . ':' . $entity_type;
    
    $plugins[$child_plugin_id] = $child_plugin;
  }
  
  return $plugins;
}

/**
 * Return a new context based on an existing context.
 */
function eck_pm_eck_edit_form_from_eck_context($context, $conf) {
  if (empty($context->data)) {
    return ctools_context_create_empty('eck_edit_form:'.$conf['keyword']);
  }

  if (isset($context->data->id)) {
    return ctools_context_create('eck_edit_form:'.$conf['keyword'], $context->data);
  }
}
