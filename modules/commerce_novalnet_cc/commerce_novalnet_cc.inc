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
 *  Script : commerce_novalnet_cc.inc
 */

/**
 * Function for get the credit card form.
 *
 * @param object $order
 * @param array $settings
 *
 * @return array
 */
function commerce_novalnet_cc_payment_form($order, $settings) {

  if ((!empty($settings['cc3d'])|| !empty($settings['force_cc3d'])) || $settings['shopping_type'] != 'one_click' || !$reference_details = commerce_novalnet_reference_tid($order->uid, 'commerce_novalnet_cc')) {
    return array();
  }

  $payment_details = unserialize($reference_details['payment_details']);
  $mask_form['from_mask'] = array(
    '#type'           => 'hidden',
    '#default_value'  => 1,
    '#attributes'     => array('id' => 'from_cc_mask'),
  );
  $mask_form['reference_tid'] = array(
    '#type'           => 'hidden',
    '#default_value'  => $reference_details['tid'],
    '#attributes'     => array('id' => 'reference_tid'),
  );
  $mask_form['mask_change_form'] = array(
    '#type'           => 'markup',
    '#markup'         => '<div class="form-item"><strong><a href="#" id="change_mask_cc">' . t('Enter new card details') . '</a></strong>' . '</div><div id="nncc_mask_form">',
  );

  $mask_form['mask_nncc_type'] = array(
    '#type'           => 'textfield',
    '#title'          => t('Type of card'),
    '#size'           => 35,
    '#value'          => $payment_details['cc_card_type'],
    '#attributes'     => array('disabled' => 'true'),
  );
  $mask_form['mask_nncc_owner'] = array(
    '#type'           => 'textfield',
    '#size'           => 35,
    '#title'          => t('Card holder name'),
    '#value'          => $payment_details['cc_holder'],
    '#attributes'     => array('disabled' => 'true'),
  );
  $mask_form['mask_nncc_number'] = array(
    '#type'           => 'textfield',
    '#title'          => t('Card number'),
    '#size'           => 35,
    '#value'          => $payment_details['cc_no'],
    '#attributes'     => array('disabled' => 'true'),
  );
  $mask_form['mask_nncc_exp_month'] = array(
    '#type'           => 'textfield',
    '#title'          => t('Expiry date'),
    '#size'           => 20,
    '#value'          => $payment_details['cc_exp_month'] . ' / ' . $payment_details['cc_exp_year'] ,
    '#attributes'     => array('disabled' => 'true'),
    '#field_suffix'   => '</div>',
  );
  return $mask_form;
}

/**
 * Function for adding additional settings.
 *
 * @param array $form
 * @param array $settings
 */
function commerce_novalnet_cc_settings_form_fields(&$form, $settings) {
  $form['shopping_type'] = array(
    '#type'          => 'select',
    '#title'         => t('Shopping type'),
    '#options'       => array(
      0             => t('None'),
      'one_click'   => t('One click shopping'),
      'zero_amount' => t('Zero amount booking'),
    ),
    '#default_value' => isset($settings['shopping_type']) ? $settings['shopping_type'] : 0,
    '#attributes'    => array('id' => 'shopping_type'),
  );
  $form['cc3d'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Enable 3D secure'),
    '#description'   => t('The 3D-Secure will be activated for credit cards. The issuing bank prompts the buyer for a password what, in turn, help to prevent a fraudulent payment. It can be used by the issuing bank as evidence that the buyer is indeed their card holder. This is intended to help decrease a risk of charge-back.'),
    '#default_value' => isset($settings['cc3d']) ? $settings['cc3d'] : 0,
  );
  $form['force_cc3d'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Force 3D secure on predefined conditions'),
    '#description'   => t("If 3D secure is not enabled in the above field, then force 3D secure process as per the 'Enforced 3D secure (as per predefined filters & settings)' module configuration at the Novalnet admin portal. If the predefined filters & settings from Enforced 3D secure module are met, then the transaction will be processed as 3D secure transaction otherwise it will be processed as non 3D secure. Please note that the 'Enforced 3D secure (as per predefined filters & settings)' module should be configured at Novalnet admin portal prior to the activation here. For further information, please refer the description of this fraud module at 'Fraud Modules' tab, below 'Projects' menu, under the selected project in Novalnet admin portal or contact Novalnet support team."),
    '#default_value' => isset($settings['force_cc3d']) ? $settings['force_cc3d'] : 0,
  );
  $form['commerce_novalnet_cc_amex'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Display AMEX logo'),
    '#description'   => t('Display AMEX logo in checkout page'),
    '#default_value' => isset($settings['commerce_novalnet_cc_amex']) ? $settings['commerce_novalnet_cc_amex'] : 0,
  );
  $form['commerce_novalnet_cc_maestro'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Display Maestro logo'),
    '#description'   => t('Display Maestro logo in checkout page'),
    '#default_value' => isset($settings['commerce_novalnet_cc_maestro']) ? $settings['commerce_novalnet_cc_maestro'] : 0,
  );
  $form['commerce_novalnet_cc_cartasi'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Display CartaSi logo'),
    '#description'   => t('Display CartaSi logo in checkout page'),
    '#default_value' => isset($settings['commerce_novalnet_cc_cartasi']) ? $settings['commerce_novalnet_cc_cartasi'] : 0,
  );
  $form['commerce_novalnet_cc_style'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('CSS settings for Credit Card iframe'),
    '#collapsible' => TRUE,
    '#collapsed'   => TRUE,
  );
  $form['commerce_novalnet_cc_style']['label_style'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Label'),
    '#description'   => t('E.g:') . 'color:#999999; background-color:#FFFFFF;',
    '#default_value' => !empty($settings['commerce_novalnet_cc_style']['label_style']) ? $settings['commerce_novalnet_cc_style']['label_style'] : '',
  );
  $form['commerce_novalnet_cc_style']['field_style'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Input'),
    '#description'   => t('E.g:') . 'color:#999999; background-color:#FFFFFF;',
    '#default_value' => !empty($settings['commerce_novalnet_cc_style']['field_style']) ? $settings['commerce_novalnet_cc_style']['field_style'] : '',
  );
  $form['commerce_novalnet_cc_style']['style_text'] = array(
    '#type'          => 'textarea',
    '#title'         => t('CSS Text'),
    '#description'   => t('E.g:') . '#idselector{color:#999999;}.classSelector{color:#000000}',
    '#default_value' => !empty($settings['commerce_novalnet_cc_style']['style_text']) ? $settings['commerce_novalnet_cc_style']['style_text'] : 'body{font-family: Georgia,"Times New Roman",Times,serif;font-size:14px;}
label{font-weight: bold;}.input-group, .label-group{width:95%;display:block}
#cvc_span, .input-group{float:left;width:35%}',
  );
  $form['commerce_novalnet_cc_style']['cc_holder'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Add style only for Card holder name'),
    '#collapsible' => TRUE,
    '#collapsed'   => TRUE,
  );
  $form['commerce_novalnet_cc_style']['cc_holder']['label_style'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Label'),
    '#description'   => t('E.g:') . 'color:#999999; background-color:#FFFFFF;',
    '#default_value' => !empty($settings['commerce_novalnet_cc_style']['cc_holder']['label_style']) ? $settings['commerce_novalnet_cc_style']['cc_holder']['label_style'] : '',
  );
  $form['commerce_novalnet_cc_style']['cc_holder']['field_style'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Input'),
    '#description'   => t('E.g:') . 'color:#999999; background-color:#FFFFFF;',
    '#default_value' => !empty($settings['commerce_novalnet_cc_style']['cc_holder']['field_style']) ? $settings['commerce_novalnet_cc_style']['cc_holder']['field_style'] : '',
  );
  $form['commerce_novalnet_cc_style']['cc_number'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Add style only for Card number'),
    '#collapsible' => TRUE,
    '#collapsed'   => TRUE,
  );
  $form['commerce_novalnet_cc_style']['cc_number']['label_style'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Label'),
    '#description'   => t('E.g:') . 'color:#999999; background-color:#FFFFFF;',
    '#default_value' => !empty($settings['commerce_novalnet_cc_style']['cc_number']['label_style']) ? $settings['commerce_novalnet_cc_style']['cc_number']['label_style'] : '',
  );
  $form['commerce_novalnet_cc_style']['cc_number']['field_style'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Input'),
    '#description'   => t('E.g:') . 'color:#999999; background-color:#FFFFFF;',
    '#default_value' => !empty($settings['commerce_novalnet_cc_style']['cc_number']['field_style']) ? $settings['commerce_novalnet_cc_style']['cc_number']['field_style'] : '',
  );
  $form['commerce_novalnet_cc_style']['cc_expiry'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Add style only for Expiry date'),
    '#collapsible' => TRUE,
    '#collapsed'   => TRUE,
  );
  $form['commerce_novalnet_cc_style']['cc_expiry']['label_style'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Label'),
    '#description'   => t('E.g:') . 'color:#999999; background-color:#FFFFFF;',
    '#default_value' => !empty($settings['commerce_novalnet_cc_style']['cc_expiry']['label_style']) ? $settings['commerce_novalnet_cc_style']['cc_expiry']['label_style'] : '',
  );
  $form['commerce_novalnet_cc_style']['cc_expiry']['field_style'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Input'),
    '#description'   => t('E.g:') . 'color:#999999; background-color:#FFFFFF;',
    '#default_value' => !empty($settings['commerce_novalnet_cc_style']['cc_expiry']['field_style']) ? $settings['commerce_novalnet_cc_style']['cc_expiry']['field_style'] : '',
  );
  $form['commerce_novalnet_cc_style']['cc_cvc'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Add style only for CVC/CVV/CID'),
    '#collapsible' => TRUE,
    '#collapsed'   => TRUE,
  );
  $form['commerce_novalnet_cc_style']['cc_cvc']['label_style'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Label'),
    '#description'   => t('E.g:') . 'color:#999999; background-color:#FFFFFF;',
    '#default_value' => !empty($settings['commerce_novalnet_cc_style']['cc_cvc']['label_style']) ? $settings['commerce_novalnet_cc_style']['cc_cvc']['label_style'] : '',
  );
  $form['commerce_novalnet_cc_style']['cc_cvc']['field_style'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Input'),
    '#description'   => t('E.g:') . 'color:#999999; background-color:#FFFFFF;',
    '#default_value' => !empty($settings['commerce_novalnet_cc_style']['cc_cvc']['field_style']) ? $settings['commerce_novalnet_cc_style']['cc_cvc']['field_style'] : '',
  );
}

/**
 * Function for render hidden form.
 *
 * @param object $order
 * @param string $payment_method
 *
 * @return form
 */
function commerce_novalnet_cc_hiddenform($order, $payment_method) {

  $data = commerce_novalnet_cc_get_request_params($order, $payment_method);
  if (!$data) return FALSE;
  foreach ($data as $param => $value) {
    $form[$param] = array(
      '#type'  => 'hidden',
      '#value' => $value,
    );
  }
  $form['#action'] = COMMERCE_NOVALNET_CC_PCI_PORT;
  return $form;
}

/**
 * Function get request params for Credit Card.
 *
 * @param object $order
 * @param array $payment_method
 * @param bool $redirect
 */
function commerce_novalnet_cc_get_request_params($order, $payment_method, $charge = '', $redirect = TRUE) {
  // Call this function for getting global merchant settings.
  $payment_method['global_setting'] = commerce_novalnet_validate_basic_data('commerce_novalnet_cc', $order->order_id);

  $charge = empty($charge) ? $order->commerce_order_total['und'][0] : $charge;
  $data   = array();
  // Call this function for getting common request param.
  commerce_novalnet_get_common_params($data, $payment_method, $order, $charge);
  $order_data = $order->data['payment_data'];

  if (!empty($order_data['nncc_hash']) && !empty($order_data['nncc_uniquid'])) {
    $data['pan_hash']  = $order_data['nncc_hash'];
    $data['unique_id'] = $order_data['nncc_uniquid'];
    $data['nn_it']     = 'iframe';
    if (!empty($payment_method['settings']['shopping_type'])) {
      $data['create_payment_ref'] = 1;
    }
  }
  elseif (!empty($order_data['payment_ref'])) {
    $data['payment_ref']   = $order_data['payment_ref'];
  }

  if ( (isset($data['payment_ref']) && empty($data['payment_ref'])) || (!empty($data['nn_it']) && (empty($data['pan_hash']) || empty($data['unique_id'])))) {
    return FALSE;
  }
  // Call this function for check zero amount booking.
  commerce_novalnet_zero_amount_check($data, $payment_method['settings']['shopping_type']);

  if ($redirect) {
    // Call this function for add re-direct params.
    commerce_novalnet_redirect_params($data, $order);
    // Function for encode the params.
    commerce_novalnet_encode_params($data);
    if (!empty($payment_method['settings']['cc3d'])) { 
      $data['cc_3d'] = 1;
    }
  }
  return $data;
}
