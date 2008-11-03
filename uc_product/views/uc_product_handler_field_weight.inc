<?php
// $Id$

/**
 * @file
 * Views handler: Product weight field.
 */

/**
 * Return a formatted weight value to display in the View.
 */
class uc_product_handler_field_weight extends views_handler_field {
  function render($values) {
    return uc_weight_format($values->{$this->field_alias});
  }
}