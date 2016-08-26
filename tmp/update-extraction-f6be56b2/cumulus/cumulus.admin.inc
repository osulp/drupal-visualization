<?php

/**
 * Implements hook_menu().
 */
function _cumulus_menu() {
  $items['admin/config/cumulus'] = array(
    'title' => 'Cumulus',
    'description' => 'Cumulus settings',
    'page callback' => 'drupal_get_form',
    'page arguments' => array('cumulus_admin_settings'),
    'access arguments' => array('administer cumulus'),
    'type' => MENU_NORMAL_ITEM,
  );
  $items['admin/structure/block/add-cumulus-block'] = array(
    'title' => 'Add cumulus block',
    'description' => 'Add a new cumulus block.',
    'access arguments' => array('administer blocks'),
    'page callback' => 'drupal_get_form',
    'page arguments' => array('cumulus_add_block_form'),
    'type' => MENU_LOCAL_ACTION,
    'file' => 'cumulus.admin.inc',
  );
  $items['admin/structure/block/delete-cumulus-block'] = array(
    'title' => 'Delete cumulus block',
    'access arguments' => array('administer blocks'),
    'page callback' => 'drupal_get_form',
    'page arguments' => array('cumulus_delete'),
    'type' => MENU_CALLBACK,
    'file' => 'cumulus.admin.inc',
  );
  return $items;
}

/**
 * Menu callback: display the cumulus block addition form.
 *
 * @see cumulus_add_block_form_submit()
 */
function cumulus_add_block_form($form, &$form_state) {
  module_load_include('inc', 'block', 'block.admin');
  return block_admin_configure($form, $form_state, 'cumulus', NULL);
}

/**
 * Save the new cumulus block.
 */
function cumulus_add_block_form_submit($form, &$form_state) {
  // Determine the delta of the new block.
  $block_ids = variable_get('cumulus_ids', array());
  $delta = empty($block_ids) ? 1 : max($block_ids) + 1;

  // Save the new array of blocks IDs.
  $block_ids[] = $delta;
  variable_set('cumulus_ids', $block_ids);

  // Save the block configuration.
  _cumulus_block_save($delta, $form_state['values']);

  // Run the normal new block submission (borrowed from block_add_block_form_submit).
  $query = db_insert('block')->fields(array('visibility', 'pages', 'custom', 'title', 'module', 'theme', 'region', 'status', 'weight', 'delta', 'cache'));
  foreach (list_themes() as $key => $theme) {
    if ($theme->status) {
      $region = !empty($form_state['values']['regions'][$theme->name]) ? $form_state['values']['regions'][$theme->name] : BLOCK_REGION_NONE;
      $query->values(array(
        'visibility' => (int) $form_state['values']['visibility'],
        'pages' => trim($form_state['values']['pages']),
        'custom' => (int) $form_state['values']['custom'],
        'title' => $form_state['values']['title'],
        'module' => $form_state['values']['module'],
        'theme' => $theme->name,
        'region' => ($region == BLOCK_REGION_NONE ? '' : $region),
        'status' => 0,
        'status' => (int) ($region != BLOCK_REGION_NONE),
        'weight' => 0,
        'delta' => $delta,
        'cache' => DRUPAL_NO_CACHE,
      ));
    }
  }
  $query->execute();

  $query = db_insert('block_role')->fields(array('rid', 'module', 'delta'));
  foreach (array_filter($form_state['values']['roles']) as $rid) {
    $query->values(array(
      'rid' => $rid,
      'module' => $form_state['values']['module'],
      'delta' => $delta,
    ));
  }
  $query->execute();

  drupal_set_message(t('The block has been created.'));
  cache_clear_all();
  $form_state['redirect'] = 'admin/structure/block';
}

/**
 * Alters the block admin form to add delete links next to cumulus blocks.
 */
function _cumulus_form_block_admin_display_form_alter(&$form, $form_state) {
  $blocks = module_invoke_all('cumulus_blocks');
  foreach (variable_get('cumulus_ids', array()) AS $delta) {
    if (empty($blocks[$delta])) {
      $form['blocks']['cumulus_' . $delta]['delete'] = array('#type' => 'link', '#title' => t('delete'), '#href' => 'admin/structure/block/delete-cumulus-block/' . $delta);
    }
  }
}

/**
 * Menu callback: confirm deletion of cumulus blocks.
 */
function cumulus_delete($form, &$form_state, $delta = 0) {
  $config = cumulus_get_config($delta);
  $title = check_plain($config['info']);
  $form['block_title'] = array('#type' => 'hidden', '#value' => $title);
  $form['delta'] = array('#type' => 'hidden', '#value' => $delta);

  return confirm_form($form, t('Are you sure you want to delete the "%name" block?', array('%name' => $title)), 'admin/structure/block', NULL, t('Delete'), t('Cancel'));
}

/**
 * Deletion of cumulus blocks.
 */
function cumulus_delete_submit($form, &$form_state) {
  // Remove the cumulus block configuration variables.
  $delta = $form_state['values']['delta'];
  $block_ids = variable_get('cumulus_ids', array());
  unset($block_ids[array_search($delta, $block_ids)]);
  sort($block_ids);

  foreach(array_keys(cumulus_default_config()) as $parameter) {
    cumulus_variable_del($parameter, $delta);
  }

  variable_set('cumulus_ids', $block_ids);

  db_delete('block')
    ->condition('module', 'cumulus')
    ->condition('delta', $delta)
    ->execute();
  db_delete('block_role')
    ->condition('module', 'cumulus')
    ->condition('delta', $delta)
    ->execute();
  drupal_set_message(t('The block "%name" has been removed.', array('%name' => $form_state['values']['block_title'])));
  cache_clear_all();
  $form_state['redirect'] = 'admin/structure/block';
  return;
}

/**
 * Implements hook_block_info().
 */
function _cumulus_block_info() {
  $blocks = array();
  $deltas = variable_get('cumulus_ids', array());
  foreach (array_keys(module_invoke_all('cumulus_blocks')) as $delta) {
    $deltas[] = $delta;
  }
  foreach ($deltas AS $delta) {
    $config = cumulus_get_config($delta);
    $blocks[$delta]['info'] = check_plain($config['info']);
    $blocks[$delta]['cache'] = DRUPAL_NO_CACHE;
  }
  return $blocks;
}

/**
 * Implements hook_block_configure().
 */
function _cumulus_block_configure($delta = '') {
  // Create a pseudo form state.
  $form_state = array('values' => cumulus_get_config($delta));
  return cumulus_configure_form(array(), $form_state);
}

/**
 * Implements hook_block_configure().
 */
function cumulus_configure_form($form, &$form_state) {
  $config = array();
  // Get the config from the form state.
  if (!empty($form_state['values'])) {
    $config = $form_state['values'];
  }
  // Merge in the default configuration.
  $config += cumulus_get_config();

  $form['info'] = array(
     '#type' => 'textfield',
     '#title' => t('Block description'),
     '#default_value' => $config['info'],
     '#maxlength' => 64,
     '#description' => t('A brief description of your block. Used on the <a href="@overview">Blocks administration page</a>.', array('@overview' => url('admin/structure/block'))),
     '#required' => TRUE,
     '#weight' => -19,
   );

  $form['vid'] = array(
    '#type' => 'textfield',
    '#title' => t('Vocabulary IDs to be included'),
    '#default_value' => $config['vid'],
    '#maxlength' => 10,
    '#description' => t('The IDs of the vocabularies that will be displayed. Separate the IDs by commas only (eg: 1, 3, 4)'),
  );

  $form['tagadelic_step'] = array(
    '#type' => 'textfield',
    '#title' => t('Tag size interval'),
    '#default_value' => $config['tagadelic_step'],
    '#maxlength' => 2,
    '#description' => t('The number of tag sizes you want to use.'),
  );

  $form['tagadelic_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of tags to display'),
    '#default_value' => $config['tagadelic_limit'],
    '#maxlength' => 2,
    '#description' => t('The maximum number of tags that will be displayed.'),
  );

  $form['flash_width'] = array(
    '#type' => 'textfield',
    '#title' => t('Width of cumulus'),
    '#default_value' => $config['flash_width'],
    '#maxlength' => 3,
    '#description' => t('The width of the cumulus in pixels.'),
  );

  $form['flash_height'] = array(
    '#type' => 'textfield',
    '#title' => t('Height of cumulus'),
    '#default_value' => $config['flash_height'],
    '#maxlength' => 3,
    '#description' => t('The height of the cumulus in pixels.'),
  );

  $form['flash_background'] = array(
    '#type' => 'textfield',
    '#title' => t('Background color of cumulus'),
    '#default_value' => $config['flash_background'],
    '#maxlength' => 6,
    '#description' => t('The hex color value for the background of the cumulus. E.g. ffffff. If "Background transparency" is enabled, this option will have no effect.'),
  );

  $form['flash_transparency'] = array(
    '#type' => 'select',
    '#title' => t('Background transparency'),
    '#default_value' => $config['flash_transparency'],
    '#options' => array(
      'false' => t('no'),
      'true' => t('yes'),
    ),
    '#description' => t('Enabling background transparency might cause issues with some (mostly older) browsers.<br />Under Linux, transparency doesn\'t work at all due to a known limitation in the current Flash player.'),
  );

  $form['flash_color'] = array(
    '#type' => 'textfield',
    '#title' => t('Font color of cumulus'),
    '#default_value' => $config['flash_color'],
    '#maxlength' => 6,
    '#description' => t('The hex color value you would like to use for the tags. E.g. 000000.'),
  );

  $form['flash_color2'] = array(
    '#type' => 'textfield',
    '#title' => t('Second font color of cumulus'),
    '#default_value' => $config['flash_color2'],
    '#maxlength' => 6,
    '#description' => t('Second tag color. If supplied, tags will get a color from a gradient between both colors based on their popularity.'),
  );

  $form['flash_hicolor'] = array(
    '#type' => 'textfield',
    '#title' => t('Highlight color of cumulus'),
    '#default_value' => $config['flash_hicolor'],
    '#maxlength' => 6,
    '#description' => t('The hex color value you would like to use for the tag mouseover/hover color'),
  );

  $form['flash_speed'] = array(
    '#type' => 'textfield',
    '#title' => t('Rotation speed'),
    '#default_value' => $config['flash_speed'],
    '#maxlength' => 3,
    '#description' => t('Set the speed of the cumulus. Options between 25 and 500 work best.'),
  );

  $form['flash_distribute'] = array(
    '#type' => 'select',
    '#title' => t('Distribute tags evenly on cumulus'),
    '#default_value' => $config['flash_distribute'],
    '#options' => array(
      'false' => t('no'),
      'true' => t('yes'),
    ),
    '#description' => t('When enabled, the movie will attempt to distribute the tags evenly over the surface of the cumulus.'),
  );

  $form['flash_font_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Font size'),
    '#default_value' => $config['flash_font_size'],
    '#maxlength' => 2,
    '#description' => t('Set the font size of the tag with the lowest tag-size in pixels (level 1).'),
  );
  $form['flash_font_size_interval'] = array(
    '#type' => 'textfield',
    '#title' => t('Font size interval'),
    '#default_value' => $config['flash_font_size_interval'],
    '#maxlength' => 1,
    '#description' => t('Set the font size interval used for the different tag-sizes (level 2 and higher).'),
  );

  return $form;
}

/**
 * Implements hook_block_save().
 */
function _cumulus_block_save($delta = '', $edit = array()) {
  if (!empty($delta)) {
    // Don't save values for an exported block.
    $config = cumulus_get_config($delta);
    if (empty($config['exported_to_code'])) {
      foreach(array_keys(cumulus_default_config()) as $parameter) {
        cumulus_variable_set($parameter, $delta, $edit[$parameter]);
      }
    }
  }
}
