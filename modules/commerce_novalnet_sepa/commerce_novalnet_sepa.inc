<?php

/**
 * @file
 * Novalnet payment method module
 * This module is used for real time processing of
 * Novalnet transaction of customers.
 *
 * @category   PHP
 * @package    commerce_novalnet
 * @author     Novalnet AG
 * @copyright  Copyright by Novalnet
 * @license    https://www.novalnet.com/payment-plugins/free/license
 * @version    11.1.2
 *
 *  Script : commerce_novalnet_sepa.inc
 */

/**
 * Function for adding back-end settings fields for SEPA.
 *
 * @param $form
 * @param $settings
 */
function commerce_novalnet_sepa_settings_form_fields(&$form, $settings) {

  $form['shopping_type'] = array(
    '#type'          => 'select',
    '#title'         => t('Shopping type'),
    '#options'       => array(0 => t('None'), 'one_click' => t('One click shopping'), 'zero_amount' => t('Zero amount booking')),
    '#default_value' => isset($settings['shopping_type']) ? $settings['shopping_type'] : 0,
  );
  $form['payment_refill'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Enable auto-fill for payment data'),
    '#description'   => t('For the registered users SEPA direct debit details will be filled automatically in the payment form'),
    '#default_value' => isset($settings['payment_refill']) ? $settings['payment_refill'] : 0,
  );
  $form['auto_refill'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Enable auto-fill'),
    '#description'   => t('The payment details will be filled automatically in the payment form during the checkout process'),
    '#default_value' => isset($settings['auto_refill']) ? $settings['auto_refill'] : 0,
  );
  $form['due_date']        = array(
    '#type'             => 'textfield',
    '#title'            => t('SEPA payment duration (in days)'),
    '#description'      => t('Enter the number of days after which the payment should be processed (must be greater than 6 days)'),
    '#default_value'    => isset($settings['due_date']) ? $settings['due_date'] : '',
    '#element_validate' => array('commerce_novalnet_element_validate_due_date'),
  );
}

/**
 * Function for build SEPA form.
 *
 * @param array $settings
 * @param object $order
 * @param array $reference_details
 * @param string $payment_type
 *
 * @return array
 */
function commerce_novalnet_sepa_payment_form($settings, $order, $reference_details, $payment_type) {
  $form = array();
  $settings['global_setting']['customer_no'] = $order->uid;
  commerce_novalnet_check_affiliate_order($settings['global_setting']);
  $global_setting = $settings['global_setting'];

  $order_wrapper   = entity_metadata_wrapper('commerce_order', $order);
  $billing_address = $order_wrapper->commerce_customer_billing->commerce_customer_address->value();

  if ($payment_type != 'guarantee_payment') {
    commerce_novalnet_fraud_prevention_form($form, $settings, $order, 'commerce_novalnet_sepa');
  }

  if (isset($form['nn_fraud_pin'])) {
    return $form;
  }
  $form['nn_sepa_hash'] = array(
    '#type'             => 'hidden',
    '#attributes'       => array('id' => 'nn_sepa_hash'),
  );
  $form['nn_sepa_refill_inprocess'] = array(
    '#type'             => 'hidden',
    '#default_value'    => '0',
    '#attributes'       => array('id' => 'nn_sepa_refill_inprocess', 'name' => ''),
  );

  $form['nnsepa_owner'] = array(
    '#type'           => 'textfield',
    '#title'          => t('Account holder') . ' <span title="' . t('This field is required.') . '" class="form-required">*</span>',
    '#default_value'  => $billing_address['name_line'],
    '#attributes'     => array('autocomplete' => 'off', 'id' => 'nnsepa_holder'),
    '#size'           => 27,
    '#maxlength'      => 64,
    '#prefix'         => '<div id="nnsepa_normal_form">',
  );
  $form['nnsepa_country'] = array(
    '#type'           => 'select',
    '#title'          => t('Bank country') . ' <span title="' . t('This field is required.') . '" class="form-required">*</span>',
    '#options'        => country_get_list(),
    '#attributes'     => array('id' => 'nnsepa_country'),
    '#default_value'  => $billing_address['country'],
  );
  $form['nnsepa_acno'] = array(
    '#type'           => 'textfield',
    '#title'          => t('IBAN or Account number') . ' <span title="' . t('This field is required.') . '" class="form-required">*</span>',
    '#default_value'  => '',
    '#attributes'     => array('autocomplete' => 'off', 'id' => 'nnsepa_acno', 'name' => ''),
    '#size'           => 27,
    '#field_suffix'   => '<div class="form-item form-type-textfield form-item-panes-payment-details-nnsepa-iban" id="nnsepa_iban_div"><label>' . t('IBAN') . ' : <span id="frm_iban"></span></label></div>',
  );
  $form['nnsepa_bcode'] = array(
    '#type'           => 'textfield',
    '#title'          => t('BIC or Bank code') . ' <span title="' . t('This field is required.') . '" class="form-required">*</span>',
    '#default_value'  => '',
    '#attributes'     => array('autocomplete' => 'off', 'id' => 'nnsepa_bcode', 'name' => ''),
    '#size'           => 27,
    '#field_suffix'   => '<div class="form-item form-type-textfield form-item-panes-payment-details-nnsepa-bic" id="nnsepa_bic_div">
    <label>' . t('BIC') . ' : <span id="frm_bic"></span></label></div>',
  );
  $form['nnsepa_ibanconf'] = array(
    '#type'           => 'checkbox',
    '#title'          => t("I hereby grant the SEPA direct debit mandate and confirm that the given IBAN and BIC are correct."),
    '#default_value'  => '',
    '#attributes'     => array('autocomplete' => 'off', 'class' => array('nnsepa_ibanconf')),
  );
  $form['nnsepa_normal_close'] = array(
    '#type'           => 'markup',
    '#markup'         => '</div>',
  );
  if (!empty($reference_details)) {
    $form = commerce_novalnet_sepa_oneclick_form($form, $reference_details);
  }

  if ($payment_type == 'guarantee_payment') {
    commerce_novalnet_dob_field($form, $settings, $order);
  }
  $form['#attached']['css'][] = array(
    'data' => drupal_get_path('module', 'commerce_novalnet_sepa') . '/commerce_novalnet_sepa.css',
    'type' => 'file',
  );
  $form['#attached']['js'][] = array(
    'data' => drupal_get_path('module', 'commerce_novalnet_sepa') . '/commerce_novalnet_sepa.js',
    'type' => 'file',
  );
  $nn_unique_id = commerce_novalnet_get_random_string();
  $form['nn_unique_id'] = array(
    '#type'             => 'hidden',
    '#default_value'    => $nn_unique_id,
    '#attributes'       => array('id' => 'nn_unique_id'),
  );
  $form['#attached']['js'][] = array(
    'data' => array(
      'nn_remote_ip'        => commerce_novalnet_get_req_ip_address('REMOTE_ADDR'),
      'nn_auth_code'        => $global_setting['auth_code'],
      'nn_vendor_id'        => $global_setting['vendor_id'],
      'nn_unique_id'        => $nn_unique_id,
      'nn_sepa_refill_hash' => commerce_novalnet_sepa_refill_hash($settings, $order->uid),
      ),
    'type' => 'setting',
  );
  $form['novalnet_sepa_markup'] = array(
    '#type'   => 'markup',
    '#weight' => '50',
    '#markup' => t('Your account will be debited upon the order submission') . commerce_novalnet_test_payment_info($settings),
  );
  return $form;
}

/**
 * Function for display on-click form.
 *
 * @param array $normal_form
 * @param array $reference_details
 *
 * @return array
 */
function commerce_novalnet_sepa_oneclick_form($normal_form, $reference_details) {
  $payment_details = unserialize($reference_details['payment_details']);

  $mask_form['from_mask'] = array(
    '#type'           => 'hidden',
    '#default_value'  => '1',
    '#attributes'     => array('id' => 'from_sepa_mask'),
  );
  $mask_form['reference_tid'] = array(
    '#type'           => 'hidden',
    '#default_value'  => $reference_details['tid'],
    '#attributes'     => array('id' => 'reference_tid'),
  );
  $mask_form['mask_change_form'] = array(
    '#type'           => 'markup',
    '#markup'         => '<div class="form-item"><strong><a href="#" id="change_mask_sepa">' . t('Enter new account details') . '</a></strong>' . '</div><div id="nnsepa_mask_form">',
  );
  $mask_form['mask_nnsepa_owner'] = array(
    '#type'           => 'textfield',
    '#title'          => t('Account holder'),
    '#value'          => $payment_details['bankaccount_holder'],
    '#attributes'     => array('disabled' => 'true'),
  );
  $mask_form['mask_nnsepa_acno'] = array(
    '#type'           => 'textfield',
    '#title'          => t('IBAN'),
    '#value'          => $payment_details['iban'],
    '#attributes'     => array('disabled' => 'true'),
  );
  if ($payment_details['bic'] == '123456') {
    $mask_form['mask_nnsepa_bcode'] = array(
      '#type'           => 'markup',
      '#markup'         => '</div>',
    );
  }
  else {
    $mask_form['mask_nnsepa_bcode'] = array(
      '#type'           => 'textfield',
      '#title'          => t('BIC'),
      '#value'          => $payment_details['bic'],
      '#attributes'     => array('disabled' => 'true'),
      '#suffix'         => '</div>',
    );
  }
  $form = array_merge($mask_form, $normal_form);
  return $form;
}

/**
 * Function for get re-fill hash for payment refill.
 *
 * @param array $settings
 * @param int $uid
 *
 * @return mixed
 */
function commerce_novalnet_sepa_refill_hash($settings, $uid) {
  global $user;
  if (!empty($settings['auto_refill'])) {
    if (isset($_SESSION['novalnet']['commerce_novalnet_sepa']['nn_sepa_hash'])) {
      return $_SESSION['novalnet']['commerce_novalnet_sepa']['nn_sepa_hash'];
    }
  }
  if (!empty($settings['payment_refill'])) {
    if ($uid > 0 && $user->uid == $uid) {
      $db_result = db_select('commerce_novalnet_transaction_detail', 'payment_details')
        ->fields('payment_details', array('payment_details', 'payment_type'))
        ->condition('customer_id', $uid)
        ->orderBy('id', 'DESC')
        ->execute()
        ->fetchAssoc();

      if (!empty($db_result) && $db_result['payment_type'] == 'commerce_novalnet_sepa') {
        $payment_details = unserialize($db_result['payment_details']);
        return isset($payment_details['sepa_hash']) ? $payment_details['sepa_hash'] : '';
      }
    }
  }
  return '';
}
