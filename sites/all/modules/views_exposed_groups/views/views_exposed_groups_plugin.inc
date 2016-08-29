<?php
/**
 * @file
 * Provides a grouped form exposed form plugin for View 3.x.
 */
class views_exposed_groups_plugin extends views_plugin_exposed_form_basic {

  function summary_title() {
    return t('Grouped form');
  }

  function option_definition() {
    $options = parent::option_definition();
    
    $options['views_exposed_groups'] = array('default' => array());
    
	return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $groups = explode("\n", $this->options['views_exposed_groups']['groups']);
    array_walk($groups, 'trim');
    $form['views_exposed_groups']['format_groups'] = array(
    	'#type' => 'select',
    	'#title' => 'Render groups as',
    	'#options' => array('vertical_tabs' => 'Vertical tabs',
    						'fieldsets_collapsed' => 'Fieldset (collapsed)',
    						'fieldsets' => 'Fieldsets(not collapsed)'),
    	'#default_value' => $this->options['views_exposed_groups']['format_groups'],
    );
    
    $form['views_exposed_groups']['groups'] = array(
    	'#type' => 'textarea',
    	'#title' => 'Groups',
    	'#description' => 'Enter a list of groups to include in this form',
    	'#default_value' => $this->options['views_exposed_groups']['groups'],
    );
    
    $weight_delta = count($this->display->handler->get_handlers('filter'));
    
    foreach ($this->display->handler->get_handlers('filter') as $filter_name => $filter) {
      if (!$filter->options['exposed']) {
        continue;
      }
      $label = $filter->options['expose']['identifier'];
      $field_label = ($filter->options['expose']['label']) ? $filter->options['expose']['label'] : $label;
      $form['views_exposed_groups']['group-'. $label]['group'] = array(
      	'#type' => 'select',
      	'#title' => 'Group for '. $field_label,
      	'#filter_field' => $filter,
      	'#options' => $groups,
        '#title_display' => 'invisible',
      	'#default_value' => $this->options['views_exposed_groups']['group-'. $label]['group']
      );
      $form['views_exposed_groups']['group-'. $label]['filter_name'] = array(
        '#type' => 'value',
        '#value' => $filter_name,
      );
      $form['views_exposed_groups']['group-'. $label]['weight'] = array(
        '#type' => 'weight',
      	'#title' => $field_label,
      	'#filter_field' => $filter,
      	'#delta' => $weight_delta,
        '#title_display' => 'invisible',
      	'#default_value' => $this->options['views_exposed_groups']['group-'. $label]['weight']
      );
    }
    $form['views_exposed_groups']['#theme'][] = 'views_exposed_groups_reorder_filter_form';
  }

  /*
   * Tweak the exposed filter form to show grouped form options.
   */
  function exposed_form_alter(&$form, &$form_state) {
    drupal_add_js(drupal_get_path('module', 'views_exposed_groups') .'/js/views_exposed_groups.js');
    $form['#theme'] = array();
    if($this->options['views_exposed_groups']['format_groups'] == 'vertical_tabs') {
  	  $form['filters'] = array(
  	      '#type' => 'vertical_tabs',
  	      '#weight' => -10,
  	  );
    }
  	else {
  	  $form['filters'] = array(
  	      '#weight' => -10,
  	  );
  	}
    $groups = explode("\n", $this->options['views_exposed_groups']['groups']);
    array_walk($groups, 'trim');
    foreach($groups as $key => $group) {
  		$form['filters'][$key] = array(
  			'#type' => 'fieldset',
  			'#title' => $group,
  	    '#collapsible' => TRUE,
  	    '#collapsed' => ($this->options['views_exposed_groups']['format_groups'] == 'fieldsets_collapsed') ? TRUE : FALSE,
  		);
  	}
  	$fields = $this->options['views_exposed_groups'];
  	unset($fields['groups']);
  	foreach($fields as $field => $group) {
  		$field = str_replace('group-', '', $field);
  		if(isset($form[$field]) && is_array($form[$field])) {
  			$title = (!is_array($form[$field . '_op']))
  			         ? $form['#info']['filter-'. $group['filter_name']]['label']
  			         : null;
  			$form['filters'][$group['group']][$field] = $form[$field] + 
  				array('#weight' => $group['weight'],
  				      '#title' => $title);
  			unset($form[$field]);
  		}
      if(isset($form[$field . '_op']) && is_array($form[$field . '_op'])) {
  			$form['filters'][$group['group']][$field . '_op'] = $form[$field . '_op'] + 
  				array('#weight' => $group['weight'],
  				      '#title' => $form['#info']['filter-'. $group['filter_name']]['label']);
  			unset($form[$field .'_op']);
  		}
    }
	$exposed_sorts = array();
	foreach ($this->view->sort as $id => $handler) {
		if ($handler->can_expose() && $handler->is_exposed()) {
			$exposed_sorts[$id] = check_plain($handler->options['expose']['label']);
		}
	}
	if (count($exposed_sorts)) {
		$form['sort_by'] = array(
			'#type' => 'select',
			'#options' => $exposed_sorts,
			'#title' => $this->options['exposed_sorts_label'],
		);
		$sort_order = array(
		'ASC' => $this->options['sort_asc_label'],
		'DESC' => $this->options['sort_desc_label'],
		);
		if (isset($form_state['input']['sort_by']) && isset($this->view->sort[$form_state['input']['sort_by']])) {
			$default_sort_order = $this->view->sort[$form_state['input']['sort_by']]->options['order'];
		} else {
			$first_sort = reset($this->view->sort);
			$default_sort_order = $first_sort->options['order'];
		}
		if (!isset($form_state['input']['sort_by'])) {
			$keys = array_keys($exposed_sorts);
			$form_state['input']['sort_by'] = array_shift($keys);
		}
		if ($this->options['expose_sort_order']) {
			$form['sort_order'] = array(
				'#type' => 'select',
				'#options' => $sort_order,
				'#title' => t('Order'),
				'#default_value' => $default_sort_order,
			);
		}
	}
	$pager = $this->view->display_handler->get_plugin('pager');
	if ($pager) {
		$pager->exposed_form_alter($form, $form_state);
		$form_state['pager_plugin'] = $pager;
	}
	$form['submit']['#weight'] = 10;
	if (isset($form['reset'])) {
		$form['reset']['#weight'] = 10;
	}
  } 
}