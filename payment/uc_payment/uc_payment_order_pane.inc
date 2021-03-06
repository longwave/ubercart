<?php
// $Id$

/**
 * @file
 * This file contains the callbacks for the payment order pane supplied with
 * Ubercart and their corresponding helper functions.
 *
 * Order panes are defined using hook_order_pane() and use a callback to handle
 * the different processes involved in order viewing/editing.  The payment order
 * pane is defined in uc_payment_order_pane() in uc_payment.module.
 */

/**
 * Handle the Payment order pane.
 */
function uc_order_pane_payment($op, $arg1) {
  switch ($op) {
    case 'view':
      if (variable_get('uc_payment_tracking', TRUE)) {
        $context = array(
          'revision' => 'formatted-original',
          'type' => 'amount',
        );
        $output = t('Balance: @balance', array('@balance' => uc_price(uc_payment_balance($arg1), $context)));
        $output .= ' ('. l(t('View'), 'admin/store/orders/'. $arg1->order_id .'/payments') .')<br />';
      }
      $method_name = _payment_method_data($arg1->payment_method, 'review');
      if (empty($method_name)) {
        $method_name = _payment_method_data($arg1->payment_method, 'name');
      }
      $output .= t('Method: @payment_method', array('@payment_method' => $method_name));
      $func = _payment_method_data($arg1->payment_method, 'callback');
      if (function_exists($func)) {
        $method_output = $func('order-view', $arg1);
        if (!empty($method_output)) {
          $output .= '<br />'. $method_output;
        }
      }
      return $output;

    case 'customer':
      $method_name = _payment_method_data($arg1->payment_method, 'review');
      if (empty($method_name)) {
        $method_name = _payment_method_data($arg1->payment_method, 'name');
      }
      $output .= t('Method: @payment_method', array('@payment_method' => $method_name));
      $func = _payment_method_data($arg1->payment_method, 'callback');
      if (function_exists($func)) {
        $method_output = $func('customer-view', $arg1);
        if (!empty($method_output)) {
          $output .= '<br />'. $method_output;
        }
      }
      return $output;

    case 'edit-form':
      drupal_add_js('misc/progress.js');
      drupal_add_js(array('defPaymentMsg' => ''), 'setting');

      $form['payment'] = array(
        '#type' => 'fieldset',
        '#title' => t("Modify 'Payment info'"),
        '#collapsible' => TRUE,
        '#collapsed' => FALSE
      );
      $methods = _payment_method_list();
      foreach ($methods as $method) {
        $options[$method['id']] = $method['name'];
      }
      $form['payment']['payment_method'] = array(
        '#type' => 'select',
        '#title' => t('Payment method'),
        '#default_value' => $arg1->payment_method,
        '#options' => (is_array($options)) ? $options : array(t('None available')),
        '#attributes' => array('onchange' => "add_order_save_hold(); get_payment_details(Drupal.settings.ucURL.adminOrders + '". $arg1->order_id ."/payment_details/' + this.value);"),
        '#disabled' => (is_array($options)) ? FALSE : TRUE,
      );
      return $form;

    case 'edit-theme':
      drupal_add_js(drupal_get_path('module', 'uc_payment') .'/uc_payment.js');

      // build a JS array of our payment methods
      $methods = _payment_method_list();
      $method_array_js = array();
      foreach ($methods as $method) {
        $method_array_js[$method['name']] = $method['id'];
      }
      drupal_add_js(array('methods' => $method_array_js), 'setting');

      // begin the order lock, also get payment details
      drupal_add_js(array('ucOrderInitiate' => TRUE), 'setting');

      $output = '<table class="order-edit-table">';
      foreach (element_children($arg1['payment']) as $field) {
        $title = $arg1['payment'][$field]['#title'];
        $arg1['payment'][$field]['#title'] = NULL;
        $output .= '<tr><td class="oet-label">'. $title .':</td><td>'
                 . drupal_render($arg1['payment'][$field]) .'</td></tr>';
      }
      $output .= '</table><div id="payment_details"></div>';
      return $output;

    case 'edit-process':
      $changes['payment_method'] = $arg1['payment_method'];
      $func = _payment_method_data($arg1['payment_method'], 'callback');
      if (function_exists($func) && ($return = $func('edit-process', $arg1)) != NULL && is_array($return)) {
        $changes = array_merge($changes, $return);
      }
      return $changes;
  }
}
