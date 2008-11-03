<?php
// $Id$

/**
 * @file
 * Attribute administration menu items.
 *
 */

// Displays a paged list and overview of existing product attributes.
function uc_attribute_admin() {
  $header = array(
    array('data' => t('Name'), 'field' => 'a.name', 'sort' => 'asc'),
    t('Required'),
    array('data' => t('Order'), 'field' => 'a.ordering'),
    t('Number of options'),
    t('Display type'),
    t('Operations'),
  );

  $display_types = _uc_attribute_display_types();

  $result = pager_query("SELECT a.aid, a.name, a.required, a.ordering, a.display, COUNT(ao.oid) AS options FROM {uc_attributes} AS a LEFT JOIN {uc_attribute_options} AS ao ON a.aid = ao.aid GROUP BY a.aid, a.name, a.ordering, a.required, a.display". tablesort_sql($header), 30, 0, "SELECT COUNT(aid) FROM {uc_attributes}");
  while ($attr = db_fetch_object($result)) {
    $ops = array(
      l(t('edit'), 'admin/store/attributes/'. $attr->aid .'/edit'),
      l(t('options'), 'admin/store/attributes/'. $attr->aid .'/options'),
      l(t('delete'), 'admin/store/attributes/'. $attr->aid .'/delete'),
    );
    $rows[] = array(
      $attr->name,
      $attr->required == 1 ? t('Yes') : t('No'),
      array('data' => $attr->ordering, 'align' => 'center'),
      array('data' => $attr->options, 'align' => 'center'),
      $display_types[$attr->display],
      implode(' ', $ops),
    );
  }

  if (count($rows) == 0) {
    $rows[] = array(
      array('data' => t('No product attributes have been added yet.'), 'colspan' => '6')
    );
  }

  $output = theme('table', $header, $rows) . theme('pager', NULL, 30)
          . l(t('Add an attribute'), 'admin/store/attributes/add');

  return $output;
}

/**
 * Form builder for product attributes.
 *
 * @ingroup forms
 * @see uc_attribute_form_validate
 * @see uc_attribute_form_submit
 */
function uc_attribute_form($form_state, $attribute = NULL) {
  // If an attribute specified, add its ID as a hidden value.

  if (!empty($attribute)) {
    $form['aid'] = array('#type' => 'hidden', '#value' => $attribute->aid);
    drupal_set_title(t('Edit attribute: %name', array('%name' => $attribute->name)));
  }

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('This name will appear to customers on product add to cart forms.'),
    '#default_value' => $attribute->name,
    '#required' => TRUE,
  );
  $form['description'] = array(
    '#type' => 'textfield',
    '#title' => t('Help text'),
    '#description' => t('<b>Optional.</b> Enter the help text that will display beneath the attribute on product add to cart forms.'),
    '#default_value' => $attribute->description,
    '#maxlength' => 255,
  );
  $form['required'] = array(
    '#type' => 'checkbox',
    '#title' => t('Make this attribute required, forcing the customer to choose an option.'),
    '#description' => t('Selecting this for an attribute will disregard any default option you specify.<br />May be overridden at the product level.'),
    '#default_value' => $attribute->required,
  );
  $form['display'] = array(
    '#type' => 'select',
    '#title' => t('Display type'),
    '#description' => t('This specifies how the options for this attribute will be presented.<br />May be overridden at the product level.'),
    '#options' => _uc_attribute_display_types(),
    '#default_value' => isset($attribute->display) ? $attribute->display : 1,
  );
  $form['ordering'] = array(
    '#type' => 'weight',
    '#title' => t('Order'),
    '#description' => t('Multiple attributes on an add to cart form are sorted by this value and then by their name.<br />May be overridden at the product level.'),
    '#default_value' => isset($attribute->ordering) ? $attribute->ordering : 0,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
    '#suffix' => l(t('Cancel'), 'admin/store/attributes'),
  );

  return $form;
}

/**
 * Submit function for uc_attribute_add_form().
 */
function uc_attribute_form_submit($form, &$form_state) {
  if (!empty($form_state['values']['aid'])) {
    db_query("UPDATE {uc_attributes} SET name = '%s', ordering = %d, required = %d, display = %d, description = '%s' WHERE aid = %d", $form_state['values']['name'], $form_state['values']['ordering'], $form_state['values']['required'], $form_state['values']['display'], $form_state['values']['description'], $form_state['values']['aid']);
  }
  else {
    db_query("INSERT INTO {uc_attributes} (name, ordering, required, display, description) VALUES ('%s', %d, %d, %d, '%s')", $form_state['values']['name'], $form_state['values']['ordering'], $form_state['values']['required'], $form_state['values']['display'], $form_state['values']['description']);
  }

  $form_state['redirect'] = 'admin/store/attributes';
}

// Confirms the deletion of the given attribute.
function uc_attribute_delete_confirm($form_state, $attribute) {
  // If we got a bunk attribute, kick out an error message.
  if (empty($attribute)) {
    drupal_set_message(t('There is no attribute with that ID.'), 'error');
    drupal_goto('admin/store/attributes');
  }

  $form['aid'] = array('#type' => 'value', '#value' => $attribute->aid);
  $form['#redirect'] = 'admin/store/attributes';

  $count = db_result(db_query("SELECT COUNT(*) FROM {uc_product_attributes} WHERE aid = %d", $attribute->aid));

  $output = confirm_form($form, t('Are you sure you want to delete the attribute %name?', array('%name' => $attribute->name)),
              'admin/store/attributes', format_plural($count, 'There is @count product with this attribute.', 'There are @count products with this attribute.'),
              t('Delete'), t('Cancel'));

  return $output;
}

/**
 * Change the display of attribute option prices.
 *
 * @ingroup forms
 */
function uc_attribute_admin_settings() {
  $form = array();

  $form['uc_attribute_option_price_format'] = array('#type' => 'radios',
    '#title' => t('Option price format'),
    '#default_value' => variable_get('uc_attribute_option_price_format', 'adjustment'),
    '#options' => array('none' => t('Do not display'),
      'adjustment' => t('Display price adjustment'),
      'total' => t('Display total price'),
    ),
    '#description' => t('Formats the price in the attribute selection form when the customer adds a product to their cart. The total price will only be displayed on products with only one price affecting attribute.'),
  );

  return system_settings_form($form);
}

function uc_attribute_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    db_query("DELETE FROM co USING {uc_class_attribute_options} AS co, {uc_attribute_options} AS ao WHERE co.oid = ao.oid AND ao.aid = %d", $form_state['values']['aid']);
    db_query("DELETE FROM {uc_class_attributes} WHERE aid = %d", $form_state['values']['aid']);
    db_query("DELETE FROM po USING {uc_product_options} AS po, {uc_attribute_options} AS ao WHERE po.oid = ao.oid AND ao.aid = %d", $form_state['values']['aid']);
    db_query("DELETE FROM pd USING {uc_product_adjustments} AS pd, {uc_product_attributes} AS pa WHERE pd.nid = pa.nid AND pa.aid = %d", $form_state['values']['aid']);
    db_query("DELETE FROM {uc_product_attributes} WHERE aid = %d", $form_state['values']['aid']);
    db_query("DELETE FROM {uc_attribute_options} WHERE aid = %d", $form_state['values']['aid']);
    db_query("DELETE FROM {uc_attributes} WHERE aid = %d", $form_state['values']['aid']);
    drupal_set_message(t('Product attribute deleted.'));
  }
}

// Display options and the modifications to products they represent.
function uc_attribute_options($attribute) {
  // If we got a bunk attribute, kick out an error message.
  if (empty($attribute)) {
    drupal_set_message(t('There is no attribute with that ID.'), 'error');
    drupal_goto('admin/store/attributes');
  }
  $aid = $attribute->aid;

  drupal_set_title(t('Options for %name', array('%name' => $attribute->name)));

  $header = array(t('Name'), t('Default cost'), t('Default price'), t('Default weight'), t('Order'), t('Operations'));

  foreach ($attribute->options as $key => $data) {
    $ops = array(
      l(t('edit'), 'admin/store/attributes/'. $aid .'/options/'. $key .'/edit'),
      l(t('delete'), 'admin/store/attributes/'. $aid .'/options/'. $key .'/delete'),
    );
    $rows[] = array($data->name, $data->cost, $data->price, $data->weight, $data->ordering, implode(' ', $ops));
  }
  if (count($rows) == 0) {
    $rows[] = array(
      array('data' => t('No options for this attribute have been added yet.'), 'colspan' => '6')
    );
  }

  $output .= theme('table', $header, $rows)
           . l(t('Add an option'), 'admin/store/attributes/'. $aid .'/options/add');

  return $output;
}

/**
 * Format an attribute and its options.
 *
 * @ingroup themeable
 */
function theme_uc_attribute_options($aid, $nid) {
  $attribute = uc_attribute_load($aid, $nid, 'product');
  $output .= '<h2>'. $attribute->name .'</h2>';
  $rows = array();
  foreach ($attribute->options as $key => $input) {
    $rows[] = array($input->oid, $input->name);
  }
  if (count($rows) == 0) {
    $rows[] = array(array('data' => t('(To be determined by customer.)'), 'colspan' => '2'));
  }
  $header = array(t('Option ID'), t('Name'));
  $output .= theme('table', $header, $rows);
  return $output;
}

/**
 * Form builder for attribute options.
 *
 * @ingroup forms
 * @see uc_attribute_option_form_validate
 * @see uc_attribute_option_form_submit
 */
function uc_attribute_option_form($form_state, $attribute, $option = NULL) {
  // If we got a bunk attribute, kick out an error message.
  if (empty($attribute)) {
    drupal_set_message(t('There is no attribute with that ID.'), 'error');
    drupal_goto('admin/store/attributes');
  }
  $aid = $attribute->aid;

  $form['aid'] = array('#type' => 'hidden', '#value' => $aid);

  if (!empty($option)) {
    $form['oid'] = array('#type' => 'hidden', '#value' => $option->oid);
    drupal_set_title(t('Edit option: %name', array('%name' => $option->name)));
  }
  else {
    drupal_set_title(t('Options for %name', array('%name' => $attribute->name)));
  }

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('This name will appear to customers on product add to cart forms.'),
    '#default_value' => $option->name,
    '#required' => TRUE,
    '#weight' => 0,
  );
  $form['ordering'] = array(
    '#type' => 'weight',
    '#title' => t('Order'),
    '#description' => t('Options will be listed sorted by this value and then by their name.<br />May be overridden at the product level.'),
    '#default_value' => isset($option->ordering) ? $option->ordering : 0,
    '#weight' => 4,
  );
  $form['adjustments'] = array(
    '#type' => 'fieldset',
    '#title' => t('Default adjustments'),
    '#description' => t('Enter a positive or negative value for each adjustment applied when this option is selected.<br />Any of these may be overriden at the product level.'),
    '#collapsible' => FALSE,
    '#weight' => 8,
  );
  $form['adjustments']['cost'] = array(
    '#type' => 'textfield',
    '#title' => t('Cost'),
    '#default_value' => $option->cost,
    '#weight' => 1,
  );
  $form['adjustments']['price'] = array(
    '#type' => 'textfield',
    '#title' => t('Price'),
    '#default_value' => $option->price,
    '#weight' => 2,
  );
  $form['adjustments']['weight'] = array(
    '#type' => 'textfield',
    '#title' => t('Weight'),
    '#default_value' => $option->weight,
    '#weight' => 3,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
    '#suffix' => l(t('Cancel'), 'admin/store/attributes/'. $aid .'/options'),
    '#weight' => 10,
  );

  return $form;
}

/**
 * Validate number formats.
 */
function uc_attribute_option_form_validate($form, &$form_state) {
  $pattern = '/^-?\d*(\.\d*)?$/';
  $price_error = t('This must be in a valid number format. No commas and only one decimal point.');
  if (!is_numeric($form_state['values']['cost']['#value']) && !preg_match($pattern, $form_state['values']['cost']['#value'])) {
    form_set_error('cost', $price_error);
  }
  if (!is_numeric($form_state['values']['price']['#value']) && !preg_match($pattern, $form_state['values']['price']['#value'])) {
    form_set_error('price', $price_error);
  }
  if (!is_numeric($form_state['values']['weight']['#value']) && !preg_match($pattern, $form_state['values']['weight']['#value'])) {
    form_set_error('weight', $price_error);
  }
}

/**
 * Submit function for uc_attribute_option_form().
 */
function uc_attribute_option_form_submit($form, &$form_state) {
  if (!isset($form_state['values']['oid'])) {
    db_query("INSERT INTO {uc_attribute_options} (aid, name, cost, price, weight, ordering) VALUES (%d, '%s', %f, %f, %f, %d)",
      $form_state['values']['aid'], $form_state['values']['name'], $form_state['values']['cost'], $form_state['values']['price'], $form_state['values']['weight'], $form_state['values']['ordering']);
    drupal_set_message(t('Created new option %option.', array('%option' => $form_state['values']['name'])));
    watchdog('uc_attribute', 'Created new option %option.', array('%option' => $form_state['values']['name']), WATCHDOG_NOTICE, 'admin/store/attributes/'. $form_state['values']['aid'] .'/options/add');
    $form_state['redirect'] = 'admin/store/attributes/'. $form_state['values']['aid'] .'/options/add';
  }
  else {
    db_query("UPDATE {uc_attribute_options} SET name = '%s', cost = %f, price = %f, weight = %f, ordering = %d WHERE aid = %d AND oid = %d",
      $form_state['values']['name'], $form_state['values']['cost'], $form_state['values']['price'], $form_state['values']['weight'], $form_state['values']['ordering'], $form_state['values']['aid'], $form_state['values']['oid']);
    drupal_set_message(t('Updated option %option.', array('%option' => $form_state['values']['name'])));
    watchdog('uc_attribute', 'Updated option %option.', array('%option' => $form_state['values']['name']), WATCHDOG_NOTICE, 'admin/store/attributes/'. $form_state['values']['aid'] .'/options/'. $form_state['values']['oid']);
    $form_state['redirect'] = 'admin/store/attributes/'. $form_state['values']['aid'] .'/options';
  }
}

// Confirms deletion of the given attribute option.
function uc_attribute_option_delete_confirm($form_state, $attribute, $option) {
  if (empty($option)) {
    drupal_set_message(t('There is no option with that ID.'), 'error');
    drupal_goto('admin/store/attributes/'. $aid .'/options');
  }
  $aid = $attribute->aid;
  $oid = $option->oid;

  $form['aid'] = array('#type' => 'value', '#value' => $aid);
  $form['oid'] = array('#type' => 'value', '#value' => $oid);

  $output = confirm_form($form, t('Are you sure you want to delete the option %name?', array('%name' => $option->name)),
             'admin/store/attributes/'. $aid .'/options', '',
             t('Delete'), t('Cancel'));

  return $output;
}

/**
 * Submit function for uc_attribute_option_delete_confirm().
 */
function uc_attribute_option_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $match = 'i:'. $form_state['values']['aid'] .';s:'. strlen($form_state['values']['oid']) .':"'. $form_state['values']['oid'] .'";';
    db_query("DELETE FROM {uc_product_adjustments} WHERE combination LIKE '%%%s%%'", $match);
    db_query("DELETE ao, co, po FROM {uc_attribute_options} AS ao LEFT JOIN {uc_product_options} AS po ON ao.oid = po.oid LEFT JOIN {uc_class_attribute_options} AS co ON ao.oid = co.oid WHERE ao.oid = %d", $form_state['values']['oid']);
  }
  $form_state['redirect'] = 'admin/store/attributes/'. $form_state['values']['aid'] .'/options';
}

// Form to associate attributes with products or classes.
function uc_object_attributes_form($form_state, $object, $type, $view = 'overview') {
  switch ($type) {
    case 'class':
      $class = $object;
      $id = $class->pcid;
      if (empty($class->name)) {
        drupal_goto('admin/store/products/classes/'. $id);
      }
      drupal_set_title(check_plain($class->name));
      $attributes = uc_class_get_attributes($id);
      break;
    case 'product':
    default:
      $product = $object;
      $id = $product->nid;
      if (empty($product->title)) {
        drupal_goto('node/'. $id);
      }
      drupal_set_title(check_plain($product->title));
      $attributes = uc_product_get_attributes($id);
  }

  $used_aids = array();
  foreach ($attributes as $attribute) {
    $used_aids[] = $attribute->aid;
  }

  if ($view == 'overview') {
    $form['#tree'] = TRUE;

    if (count($attributes) > 0) {
      foreach ($attributes as $attribute) {
        $option = $attribute->options[$attribute->default_option];

        $form['attributes'][$attribute->aid] = array(
          'remove' => array(
            '#type' => 'checkbox',
            '#default_value' => 0,
          ),
          'name' => array(
            '#value' => $attribute->name,
          ),
          'option' => array(
            '#value' => $option ? ($option->name .' ('. uc_currency_format($option->price) .')' ) : t('n/a'),
          ),
          'required' => array(
            '#type' => 'checkbox',
            '#default_value' => $attribute->required,
          ),
          'ordering' => array(
            '#type' => 'weight',
            '#default_value' => $attribute->ordering,
          ),
          'display' => array(
            '#type' => 'select',
            '#default_value' => $attribute->display,
            '#options' => _uc_attribute_display_types(),
          ),
        );
      }

      $form['save'] = array(
        '#type' => 'submit',
        '#value' => t('Save changes'),
        '#weight' => -2,
      );
    }
  }
  else if ($view == 'add') {
    // Get list of attributes not already assigned to this node or class.
    $unused_attributes = array();
    $result = db_query("SELECT a.aid, a.name FROM {uc_attributes} AS a LEFT JOIN {uc_attribute_options} AS ao ON a.aid = ao.aid GROUP BY a.aid, a.name ORDER BY a.name");
    while ($attribute = db_fetch_object($result)) {
      if (!in_array($attribute->aid, $used_aids)) {
        $unused_attributes[$attribute->aid] = $attribute->name;
      }
    }

    $form['add_attributes'] = array(
      '#type' => 'select',
      '#title' => t('Attributes'),
      '#description' => t('Hold Ctrl + click to select multiple attributes.'),
      '#options' => count($unused_attributes) > 0 ? $unused_attributes : array(t('No attributes left to add.')),
      '#disabled' => count($unused_attributes) == 0 ? TRUE : FALSE,
      '#multiple' => TRUE,
      '#weight' => -1
    );
    $form['add'] = array(
      '#type' => 'submit',
      '#value' => t('Add attributes'),
      '#suffix' => l(t('Cancel'), $type == 'product' ? 'node/'. $id .'/edit/attributes' : 'admin/store/products/classes/'. $class->pcid .'/attributes'),
      '#weight' => 0,
    );
  }

  $form['id'] = array(
    '#type' => 'value',
    '#value' => $id,
  );
  $form['type'] = array(
    '#type' => 'value',
    '#value' => $type,
  );
  $form['view'] = array(
    '#type' => 'value',
    '#value' => $view,
  );

  return $form;
}

/**
 * Display the formatted attribute form.
 *
 * @ingroup themeable
 */
function theme_uc_object_attributes_form($form) {
  if ($form['view']['#value'] == 'overview') {
    $header = array(t('Remove'), t('Name'), t('Default'), t('Required'), t('Order'), t('Display'));

    if (count(element_children($form['attributes'])) > 0) {
      foreach (element_children($form['attributes']) as $aid) {
        $rows[] = array(
          drupal_render($form['attributes'][$aid]['remove']),
          drupal_render($form['attributes'][$aid]['name']),
          drupal_render($form['attributes'][$aid]['option']),
          drupal_render($form['attributes'][$aid]['required']),
          drupal_render($form['attributes'][$aid]['ordering']),
          drupal_render($form['attributes'][$aid]['display']),
        );
      }
    }
    else {
      $rows[] = array(
        array('data' => t('You must first <a href="!url">add attributes to this !type</a>.', array('!url' => request_uri() .'/add', '!type' => $form['type']['#value'])), 'colspan' => 6),
      );
    }

    $output = theme('table', $header, $rows);
  }

  $output .= drupal_render($form);

  return $output;
}

function uc_object_attributes_form_submit($form, &$form_state) {
  if ($form_state['values']['type'] == 'product') {
    $attr_table = '{uc_product_attributes}';
    $opt_table = '{uc_product_options}';
    $id = 'nid';
    $sql_type = '%d';
  }
  else if ($form_state['values']['type'] == 'class') {
    $attr_table = '{uc_class_attributes}';
    $opt_table = '{uc_class_attribute_options}';
    $id = 'pcid';
    $sql_type = "'%s'";
  }

  if ($form_state['values']['view'] == 'overview' && is_array($form_state['values']['attributes'])) {
    foreach ($form_state['values']['attributes'] as $aid => $attribute) {
      if ($attribute['remove']) {
        $remove_aids[] = $aid;
      }
      else {
        db_query("UPDATE $attr_table SET ordering = %d, required = %d, display = %d WHERE aid = %d AND $id = $sql_type", $attribute['ordering'], $attribute['required'], $attribute['display'], $aid, $form_state['values']['id']);
        $changed = TRUE;
      }
    }

    if (count($remove_aids) > 0) {
      $values = array($form_state['values']['id'], implode(', ', $remove_aids));

      db_query("DELETE po FROM $opt_table AS po LEFT JOIN {uc_attribute_options} AS ao ON po.oid = ao.oid WHERE po.$id = $sql_type AND ao.aid IN (%s)", $values);
      db_query("DELETE FROM $attr_table WHERE $id = $sql_type AND aid IN (%s)", $values);
      if ($form_state['values']['type'] == 'product') {
        db_query("DELETE FROM {uc_product_adjustments} WHERE nid = %d", $form_state['values']['id']);
      }

      drupal_set_message(format_plural(count($remove_aids), '@count attribute has been removed.', '@count attributes have been removed.'));
    }

    if ($changed) {
      drupal_set_message(t('The changes have been saved.'));
    }
  }
  else if ($form_state['values']['view'] == 'add') {
    foreach ($form_state['values']['add_attributes'] as $aid) {
      db_query("INSERT INTO $attr_table ($id, aid, ordering, default_option, required, display) SELECT $sql_type, aid, ordering, 0, required, display FROM {uc_attributes} WHERE aid = %d", $form_state['values']['id'], $aid);
    }
    if (count($form_state['values']['add_attributes']) > 0) {
      if ($form_state['values']['type'] == 'product') {
        db_query("DELETE FROM {uc_product_adjustments} WHERE nid = %d", $form_state['values']['id']);
      }
      drupal_set_message(format_plural(count($form_state['values']['add_attributes']), '@count attribute has been added.', '@count attributes have been added.'));
    }
  }

  if ($form_state['values']['type'] == 'product') {
    $form_state['redirect'] = 'node/'. $form_state['values']['id'] .'/edit/attributes';
  }
  else {
    $form_state['redirect'] = 'admin/store/products/classes/'. $form_state['values']['id'] .'/attributes';
  }
}

// Form to assign and modify attribute options on products or classes.
function uc_object_options_form($form_state, $object, $type) {
  if ($type == 'product') {
    $product = $object;
    $id = $product->nid;
    drupal_set_title(check_plain($product->title));
    $attributes = uc_product_get_attributes($id);
    $table = '{uc_product_options}';
    $id_type = 'nid';
    $sql_type = db_type_placeholder('int');
  }
  else if ($type == 'class') {
    $class = $object;
    $id = $class->pcid;
    drupal_set_title(check_plain($class->name));
    $attributes = uc_class_get_attributes($id);
    $table = '{uc_class_attribute_options}';
    $id_type = 'pcid';
    $sql_type = db_type_placeholder('varchar');
  }

  foreach ($attributes as $aid => $attribute) {
    $form['attributes'][$aid] = array(
      '#prefix' => '<tr>',
      '#suffix' => '</tr>',
    );
    $form['attributes'][$aid]['name'] = array(
      '#value' => $attribute->name,
    );
    $form['attributes'][$aid]['aid'] = array(
      '#type' => 'hidden',
      '#value' => $attribute->aid,
    );
    $form['attributes'][$aid]['ordering'] = array(
      '#type' => 'value',
      '#value' => $attribute->ordering,
    );

    $form['attributes'][$aid]['options'] = array('#weight' => 2);

    $base_attr = uc_attribute_load($attribute->aid);

    if ($base_attr->options) {
      $options = array();

      $result = db_query("SELECT ao.aid, ao.oid, ao.name, ao.cost AS default_cost, ao.price AS default_price, ao.weight AS default_weight, ao.ordering AS default_ordering, po.cost, po.price, po.weight, po.ordering FROM {uc_attribute_options} AS ao LEFT JOIN $table AS po ON ao.oid = po.oid AND po.". $id_type ." = ". $sql_type ." WHERE aid = %d ORDER BY po.ordering, default_ordering, ao.name", $id, $attribute->aid);
      while ($option = db_fetch_object($result)) {
        $oid = $option->oid;
        $options[$oid] = '';

        $form['attributes'][$aid]['options'][$oid]['select'] = array(
          '#type' => 'checkbox',
          '#default_value' => isset($attribute->options[$oid]) ? TRUE : FALSE,
          '#title' => $option->name,
        );
        $form['attributes'][$aid]['options'][$oid]['cost'] = array(
          '#type' => 'textfield',
          '#default_value' => is_null($option->cost) ? $option->default_cost : $option->cost,
          '#size' => 6,
        );
        $form['attributes'][$aid]['options'][$oid]['price'] = array(
          '#type' => 'textfield',
          '#default_value' => is_null($option->price) ? $option->default_price : $option->price,
          '#size' => 6,
        );
        $form['attributes'][$aid]['options'][$oid]['weight'] = array(
          '#type' => 'textfield',
          '#default_value' => is_null($option->weight) ? $option->default_weight : $option->weight,
          '#size' => 5,
        );
        $form['attributes'][$aid]['options'][$oid]['ordering'] = array(
          '#type' => 'weight',
          '#default_value' => is_null($option->ordering) ? $option->default_ordering : $option->ordering,
        );
      }

      $form['attributes'][$aid]['default'] = array(
        '#type' => 'radios',
        '#options' => $options,
        '#default_value' => /* $attribute->required ? NULL : */ $attribute->default_option,
        //'#disabled' => $attribute->required,
      );
    }
    else {
      $form['attributes'][$aid]['default'] = array(
        '#value' => t('This attribute does not have any options.'),
      );
    }
  }

  if (!empty($form['attributes'])) {
    $form['attributes']['#tree'] = TRUE;

    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
      '#weight' => 10,
    );
  }

  $form['id'] = array(
    '#type' => 'value',
    '#value' => $id,
  );
  $form['type'] = array(
    '#type' => 'value',
    '#value' => $type,
  );

  return $form;
}

/**
 * Display the option form.
 *
 * @ingroup themeable
 */
function theme_uc_object_options_form($form) {
  $header = array(t('Attribute'), t('Options'), t('Default'), t('Cost'), t('Price'), t('Weight'), t('Order'));

  foreach (element_children($form['attributes']) as $key) {
    $row = array();
    $row[] = array('data' => drupal_render($form['attributes'][$key]['aid']) . drupal_render($form['attributes'][$key]['name']), 'class' => 'attribute');

    if (element_children($form['attributes'][$key]['default'])) {
      $first = TRUE;
      foreach (element_children($form['attributes'][$key]['default']) as $oid) {
        $row[] = drupal_render($form['attributes'][$key]['options'][$oid]['select']);
        $row[] = drupal_render($form['attributes'][$key]['default'][$oid]);
        $row[] = drupal_render($form['attributes'][$key]['options'][$oid]['cost']);
        $row[] = drupal_render($form['attributes'][$key]['options'][$oid]['price']);
        $row[] = drupal_render($form['attributes'][$key]['options'][$oid]['weight']);
        $row[] = drupal_render($form['attributes'][$key]['options'][$oid]['ordering']);

        if (!$first) {
          $row = array_pad($row, -7, '');
        }
        else {
          $first = FALSE;
        }

        $rows[] = $row;
        $row = array();
      }
      unset($form['attributes'][$key]['default']);
    }
    else {
      $row[] = array('data' => drupal_render($form['attributes'][$key]['default']), 'colspan' => 7);
      $rows[] = $row;
    }

    $rows[] = array(array('data' => '<hr />', 'colspan' => 7));
  }

  if (count($rows) == 0) {
    $rows[] = array(
      array('data' => t('This !type does not have any attributes.', array('!type' => $form['type']['#value'] == 'product' ? t('product') : t('product class'))), 'colspan' => 7),
    );
  }

  $output = theme('table', $header, $rows, array('class' => 'product_attributes'))
          . drupal_render($form);

  return $output;
}

function uc_object_options_form_validate($form, &$form_state) {
  if (isset($form_state['values']['attributes'])) {
    foreach ($form_state['values']['attributes'] as $aid => $attribute) {
      $selected_opts = array();
      if (is_array($attribute['options'])) {
        foreach ($attribute['options'] as $oid => $option) {
          if ($option['select'] == 1) {
            $selected_opts[] = $oid;
          }
        }
      }
      if (!empty($selected_opts) && !$form['attributes'][$aid]['default']['#disabled'] && !in_array($attribute['default'], $selected_opts)) {
        form_set_error($attribute['default']);
        $error = TRUE;
      }
    }
  }

  if ($error) {
    drupal_set_message(t('All attributes with enabled options must specify an enabled option as default.'), 'error');
  }
}

function uc_object_options_form_submit($form, &$form_state) {
  if ($form_state['values']['type'] == 'product') {
    $attr_table = '{uc_product_attributes}';
    $opt_table = '{uc_product_options}';
    $id = 'nid';
    $sql_type = '%d';
  }
  else if ($form_state['values']['type'] == 'class') {
    $attr_table = '{uc_class_attributes}';
    $opt_table = '{uc_class_attribute_options}';
    $id = 'pcid';
    $sql_type = "'%s'";
  }

  foreach ($form_state['values']['attributes'] as $attribute) {
    db_query("UPDATE $attr_table SET default_option = %d WHERE $id = $sql_type AND aid = %d", $attribute['default'], $form_state['values']['id'], $attribute['aid']);

    if (is_array($attribute['options'])) {
      foreach ($attribute['options'] as $oid => $option) {
        db_query("DELETE FROM $opt_table WHERE $id = $sql_type AND oid = %d", $form_state['values']['id'], $oid);

        if ($option['select']) {
          db_query("INSERT INTO $opt_table ($id, oid, cost, price, weight, ordering) VALUES ($sql_type, %d, %f, %f, %f, %d)",
                   $form_state['values']['id'], $oid, $option['cost'], $option['price'], $option['weight'], $option['ordering']);
        }
        else if ($form_state['values']['type'] == 'product') {
          $aid = $attribute['aid'];
          $match = 'i:'. $aid .';s:'. strlen($oid) .':"'. $oid .'";';
          db_query("DELETE FROM {uc_product_adjustments} WHERE nid = %d AND combination LIKE '%%%s%%'", $form_state['values']['id'], $match);
        }
      }
    }
  }

  drupal_set_message(t('The !type options have been saved.', array('!type' => $form_state['values']['type'] == 'product' ? t('product') : t('product class'))));
}

/**
 * Form builder to associate option combinations with mutations of a product's model number.
 *
 * @ingroup forms
 * @see uc_product_adjustments_form_submit
 */
function uc_product_adjustments_form($form_state, $node) {
  drupal_set_title(check_plain($node->title));
  $nid = $node->nid;

  //Populate table and such.
  $model = $node->model;
  $query_select = "SELECT DISTINCT";
  $query_from = " FROM";
  $query_where = " WHERE";
  $query_order = " ORDER BY";
  $result = db_query("SELECT pa.nid, pa.aid, pa.ordering, a.name, a.ordering, ao.aid, COUNT(po.oid) FROM {uc_product_attributes} AS pa LEFT JOIN {uc_attributes} AS a ON pa.aid = a.aid LEFT JOIN {uc_attribute_options} AS ao ON a.aid = ao.aid LEFT JOIN {uc_product_options} AS po ON ao.oid = po.oid AND po.nid = %d WHERE pa.nid = %d GROUP BY ao.aid, pa.aid, a.name, pa.ordering, a.ordering HAVING count(po.oid) > 0 ORDER BY pa.ordering, a.ordering", $nid, $nid);
  $i = 1;
  $attribute_names = '';
  $full_attributes = array();
  $values = array();

  while ($prod_attr = db_fetch_object($result)) {
    $query_select .= " ao$i.aid AS aid$i, ao$i.name AS name$i, ao$i.oid AS oid$i, po$i.ordering,";
    $query_from .= " ({uc_product_options} AS po$i LEFT JOIN {uc_attribute_options} AS ao$i ON po$i.oid = ao$i.oid AND po$i.nid = %d),";
    $values[] = $nid;
    $query_where .= " ao$i.aid = ". $prod_attr->aid ." AND";
    $query_order .= " po$i.ordering, ao$i.name,";
    ++$i;
    $attribute_names .= '<th>'. $prod_attr->name .'</th>';
    $attribute_ids[] = $prod_attr->aid;
  }
  $num_prod_attr = count($attribute_ids);

  // Remove last connecting parts (commas, "AND")
  $query_select = rtrim($query_select, ',');
  $query_from = rtrim($query_from, ',');
  $query_where = substr($query_where, 0, strlen($query_where) - 4);
  $query_order = rtrim($query_order, ',');

  if ($num_prod_attr) {
    //Get previous values
    $result = db_query("SELECT * FROM {uc_product_adjustments} WHERE nid = %d", $nid);
    $old_vals = array();
    while ($obj = db_fetch_object($result)) {
      $old_vals[] = $obj;
    }

    $result = pager_query($query_select . $query_from . $query_where . $query_order, 20, 0, NULL, $values);

    $form['original'] = array(
      '#value' => '<div><br /><b>'. t('Default product SKU: @sku', array('@sku' => $model)) .'</b></div>',
    );
    $form['default'] = array(
      '#type' => 'value', '#value' => $model,
    );
    $form['table'] = array(
      '#prefix' => '<table class="combinations">',
      '#suffix' => '</table>',
    );
    $form['table']['head'] = array(
      '#prefix' => '<thead><tr>',
      '#suffix' => '</tr></thead>',
      '#value' => $attribute_names .'<th>Alternate SKU</th>',
      '#weight' => 0,
    );
    $form['table']['body'] = array(
      '#prefix' => '<tbody>',
      '#suffix' => '</tbody>',
      '#weight' => 1,
      '#tree' => TRUE,
    );

    $i = 0;
    while ($combo = db_fetch_object($result)) {
      $cells = '';
      $row_title = '';
      $comb_array = array();
      for ($j = 1; $j <= $num_prod_attr; ++$j) {
        $cells .= '<td>'. $combo->{'name'. $j} .'</td>';
        $row_title .= $combo->{'name'. $j} .', ';
        $comb_array[$combo->{'aid'. $j}] = $combo->{'oid'. $j};
      }
      $row_title = substr($row_title, 0, strlen($row_title) - 2);
      $serial_array = serialize($comb_array);
      $default_model = $model;
      foreach ($old_vals as $ov) {
        if (!count(array_diff_assoc(unserialize($ov->combination), unserialize($serial_array)))) {
          $default_model = $ov->model;
          break;
        }
      }

      $form['table']['body'][$i] = array(
        '#prefix' => '<tr title="'. $row_title .'">',
        '#suffix' => '</tr>',
      );
      $form['table']['body'][$i]['combo'] = array(
        '#value' => $cells,
      );
      $form['table']['body'][$i]['combo_array'] = array(
        '#type' => 'value',
        '#value' => $serial_array,
      );
      $form['table']['body'][$i]['model'] = array(
        '#type' => 'textfield',
        '#default_value' => $default_model,
        '#prefix' => '<td>',
        '#suffix' => '</td>',
      );
      ++$i;
    }

    $form['nid'] = array(
      '#type' => 'hidden',
      '#value' => $nid,
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
    );
  }
  else {
    $form['error'] = array(
      '#value' => '<div><br />'. t('This product does not have any attributes.') .'</div>',
    );
  }

  $form['pager'] = array(
    '#value' => theme('pager'),
  );

  return $form;
}

function uc_product_adjustments_form_submit($form, &$form_state) {
  foreach ($form_state['values']['body'] as $value) {
    if (!empty($value['model']) && $value['model'] != $form_state['values']['default']) {
      db_query("UPDATE {uc_product_adjustments} SET model = '%s' WHERE nid = %d AND combination = '%s'",
        $value['model'], $form_state['values']['nid'], $value['combo_array']);
      if (!db_affected_rows()) {
        db_query("INSERT INTO {uc_product_adjustments} (nid, combination, model) VALUES (%d, '%s', '%s')",
          $form_state['values']['nid'], $value['combo_array'], $value['model']);
      }
    }
    else {
      db_query("DELETE FROM {uc_product_adjustments} WHERE nid = %d AND combination = '%s'",
        $form_state['values']['nid'], $value['combo_array']);
    }
  }
  drupal_set_message(t('Product adjustments have been saved.'));
  $goto = array($_GET['q']);
  if ($_GET['page']) {
    $goto[] = 'page='. $_GET['page'];
  }
  $form_state['redirect'] = $goto;
}
