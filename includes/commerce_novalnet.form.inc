<?php

 /**
  * @file
  * Novalnet payment method module
  * This module is used for real time processing of
  * Novalnet transaction of customers.
  *
  * Copyright (c) Novalnet AG.
  *
  * Released under the GNU General Public License
  * This free contribution made by request.
  * If you have found this script useful a small
  * recommendation as well as a comment on merchant form
  * would be greatly appreciated.
  *
  *  Script : commerce_novalnet.form.inc
  */

module_load_include('inc', 'commerce_novalnet', 'includes/commerce_novalnet.admin');

/**
 * Novalnet global configuration menu callback.
 *
 * @param array $form
 * @param array $form_state
 *
 * @return array
 */
function commerce_novalnet_global_config_form($form, &$form_state) {
  $admin_portal_url = base_path() . 'admin/commerce/config/novalnet-settings/admin-portal';
  $form['novalnet_admin']    = array(
    '#type'       => 'markup',
    '#markup'      => t("For additional configurations login to <a href='@admin_url'>Novalnet Merchant Administration Portal</a>. To login to the Portal you need to have an account at Novalnet. If you don't have one yet, please contact <a href='mailto:sales@novalnet.de'>sales@novalnet.de</a> / tel. +49 (089) 923068320", array('@admin_url' => $admin_portal_url)) . '<br>' . t("To use the PayPal payment method please enter your PayPal API details in <a href='@admin_url'>Novalnet Merchant Administration portal</a>", array('@admin_url' => $admin_portal_url)),
  );
  $form['novalnet_settings'] = array(
    '#type'        => 'vertical_tabs',
  );
  $form['global_config'] = array(
    '#type'        => 'fieldset',
    '#title'       => t('Global Configuration'),
    '#collapsible' => TRUE,
    '#collapsed'   => TRUE,
    '#group'       => 'novalnet_settings',
  );
  $form['global_config']['ajax'] = array(
    '#type'        => 'fieldset',
    '#collapsed'   => TRUE,
    '#tree'        => TRUE,
    '#prefix'           => '<div id="global-wrapper">',
    '#suffix'           => '</div>',
  );
  $global_setting = variable_get('novalnet_global_settings', '');
  // Core function for get the available order status.
  $order_status   = commerce_order_status_options_list();
  $form['global_config']['ajax']['nn_logo'] = array(
    '#type'             => 'checkbox',
    '#title'            => t('Display payment method logo'),
    '#description'      => t('The payment method logo will be displayed on the checkout page.'),
    '#default_value'    => isset($global_setting['nn_logo']) ? $global_setting['nn_logo'] : '1',
  );
  $form['global_config']['ajax']['public_key'] = array(
    '#type'             => 'textfield',
    '#title'            => t('Product activation key'),
    '#attributes'       => array('autocomplete' => 'off'),
    '#description'      => t('Enter Novalnet Product activation key'),
    '#ajax'             => array(
      'event' => 'change',
      'wrapper' => 'global-wrapper',
      'callback' => 'commerce_novalnet_ajax_callback',
      'effect' => 'fade',
    ),
    '#default_value'    => isset($global_setting['public_key']) ? $global_setting['public_key'] : '',
    '#required'         => TRUE,
    '#element_validate' => array('commerce_novalnet_element_validate_required'),
  );
  $form['global_config']['ajax']['vendor_id'] = array(
    '#type'             => 'textfield',
    '#title'            => t('Merchant ID'),
    '#default_value'    => isset($global_setting['vendor_id']) ? $global_setting['vendor_id'] : '',
    '#attributes'       => array('readonly' => 'readonly'),
    '#required'         => TRUE,
    '#element_validate' => array('commerce_novalnet_element_validate_required'),
  );
  $form['global_config']['ajax']['auth_code'] = array(
    '#type'             => 'textfield',
    '#title'            => t('Authentication code'),
    '#attributes'       => array('readonly' => 'readonly'),
    '#default_value'    => isset($global_setting['auth_code']) ? $global_setting['auth_code'] : '',
    '#required'         => TRUE,
    '#element_validate' => array('commerce_novalnet_element_validate_required'),
  );
  $form['global_config']['ajax']['product_id'] = array(
    '#type'             => 'textfield',
    '#title'            => t('Project ID'),
    '#attributes'       => array('readonly' => 'readonly'),
    '#default_value'    => isset($global_setting['product_id']) ? $global_setting['product_id'] : '',
    '#required'         => TRUE,
    '#element_validate' => array('commerce_novalnet_element_validate_required'),
  );
  $form['global_config']['ajax']['tariff_id'] = array(
    '#type'             => 'select',
    '#description'      => t('Select Novalnet tariff ID'),
    '#title'            => t('Tariff ID'),
    '#attributes'       => array('style' => 'width:200px'),
    '#required'         => TRUE,
    '#element_validate' => array('commerce_novalnet_element_validate_required'),
    '#validated' => TRUE,
    '#options'          => isset($global_setting['normal_tariffs']) ? $global_setting['normal_tariffs'] : array(),
    '#default_value'    => isset($global_setting['tariff_id']) ? $global_setting['tariff_id'] : '',
  );
  $form['global_config']['ajax']['payment_access_key'] = array(
    '#type'             => 'textfield',
    '#title'            => t('Payment access key'),
    '#attributes'       => array('readonly' => 'readonly'),
    '#default_value'    => isset($global_setting['payment_access_key']) ? $global_setting['payment_access_key'] : '',
    '#required'         => TRUE,
    '#element_validate' => array('commerce_novalnet_element_validate_required'),
    '#element_validate' => array('commerce_novalnet_element_validate_required'),
  );
  $form['global_config']['ajax']['manual_amount'] = array(
    '#type'             => 'textfield',
    '#size'             => 20,
    '#title'            => t('Set a limit for on-hold transaction (in minimum unit of currency. E.g. enter 100 which is equal to 1.00)'),
    '#description'      => t('In case the order amount exceeds mentioned limit, the transaction will be set on hold till your confirmation of transaction'),
    '#default_value'    => isset($global_setting['manual_amount']) ? $global_setting['manual_amount'] : '',
    '#element_validate' => array('commerce_novalnet_element_validate_number'),
  );

  $subs_payments   = array(
    'commerce_novalnet_cc'         => t('Credit Card'),
    'commerce_novalnet_sepa'       => t('Direct Debit SEPA'),
    'commerce_novalnet_invoice'    => t('Invoice'),
    'commerce_novalnet_prepayment' => t('Prepayment'),
    'commerce_novalnet_paypal'     => t('Paypal'),
  );
  $subscription_default = 1;
  $subscription_disabled = FALSE;
  $subscription_desc     = '';

  if (!module_exists('commerce_recurring')) {
    $subscription_default = 0;
    $subscription_desc    = t('Requires: Commerce Recurring Framework');
    $subscription_disabled = TRUE;
    $global_setting['subscription'] = 0;
  }
  $form['global_config']['ajax']['subscription'] = array(
    '#type'             => 'checkbox',
    '#title'            => t('Enable subscription'),
    '#disabled'         => $subscription_disabled,
    '#description'      => filter_xss($subscription_desc),
    '#default_value'    => isset($global_setting['subscription']) ? $global_setting['subscription'] : $subscription_default,
  );
  $form['global_config']['ajax']['subs'] = array(
    '#type'             => 'fieldset',
    '#title'            => t('Subscription configuration'),
    '#states'           => array('visible' => array('input[name="ajax[subscription]"]' => array("checked" => TRUE))),
  );
  $form['global_config']['ajax']['subs']['subs_tariff'] = array(
    '#type'             => 'select',
    '#description'      => t('Select Novalnet subscription tariff ID'),
    '#title'            => t('Subscription tariff ID'),
    '#attributes'       => array('style' => 'width:200px'),
    '#validated'        => TRUE,
    '#options'          => isset($global_setting['subscription_tariffs']) ? $global_setting['subscription_tariffs'] : array(),
    '#default_value'    => isset($global_setting['subs']['subs_tariff']) ? $global_setting['subs']['subs_tariff'] : '',
  );
  $form['global_config']['ajax']['subs']['subscription_payments'] = array(
    '#type'          => 'select',
    '#title'         => t('Subscription payments'),
    '#options'       => $subs_payments,
    '#multiple'      => TRUE,
    '#default_value' => isset($global_setting['subs']['subscription_payments']) ? $global_setting['subs']['subscription_payments'] : '',
  );
  $form['global_config']['ajax']['subs']['cancel_status'] = array(
    '#type'          => 'select',
    '#title'         => t('Cancellation status of subscription'),
    '#options'       => $order_status,
    '#default_value' => isset($global_setting['subs']['cancel_status']) ? $global_setting['subs']['cancel_status'] : 'canceled',
  );
  $form['global_config']['ajax']['onhold_transaction_status'] = array(
    '#type'          => 'select',
    '#title'         => t('Confirmation order status'),
    '#options'       => $order_status,
    '#default_value' => isset($global_setting['onhold_transaction_status']) ? $global_setting['onhold_transaction_status'] : 'pending',
  );
  $form['global_config']['ajax']['onhold_void_cancel_status'] = array(
    '#type'          => 'select',
    '#title'         => t('Cancellation order status'),
    '#options'       => $order_status,
    '#default_value' => isset($global_setting['onhold_void_cancel_status']) ? $global_setting['onhold_void_cancel_status'] : 'canceled',
  );
  $form['global_config']['ajax']['referrer_id'] = array(
    '#type'             => 'textfield',
    '#title'            => t('Referrer ID'),
    '#description'      => t('Enter the referrer ID of the person/company who recommended you Novalnet'),
    '#default_value'    => isset($global_setting['referrer_id']) ? $global_setting['referrer_id'] : '',
    '#element_validate' => array('commerce_novalnet_element_validate_number'),
  );
  $form['global_config']['ajax']['test_notify'] = array(
    '#type'             => 'checkbox',
    '#title'            => t('Enable E-mail notification for test transaction'),
    '#description'      => t('You will receive email notifications about every test order in the web shop.'),
    '#default_value'    => isset($global_setting['test_notify']) ? $global_setting['test_notify'] : 0,
  );
  if (!empty($global_setting) && count($global_setting) > 1) {
    commerce_novalnet_ajax_callback($form, $form_state);
  }

  $callback_setting = variable_get('novalnet_callback_settings', '');

  $form['callback_config'] = array(
    '#type'        => 'fieldset',
    '#title'       => t('Merchant script management'),
    '#collapsible' => TRUE,
    '#collapsed'   => TRUE,
    '#tree'        => TRUE,
    '#group'       => 'novalnet_settings',
  );
  $form['callback_config']['callback_mail_send'] = array(
    '#type'           => 'checkbox',
    '#title'          => t('Enable E-mail notification for callback'),
    '#default_value'  => (!empty($callback_setting)) ? $callback_setting['callback_mail_send'] : 0,
  );
  $form['callback_config']['callback_test_mode'] = array(
    '#type'           => 'checkbox',
    '#title'            => t('Enable test mode'),
    '#default_value'    => (!empty($callback_setting)) ? $callback_setting['callback_test_mode'] : 0,
  );
  $form['callback_config']['callback_debug_mode'] = array(
    '#type'           => 'checkbox',
    '#title'            => t('Enable debug mode'),
    '#default_value'    => (!empty($callback_setting)) ? $callback_setting['callback_debug_mode'] : 0,
  );
  $form['callback_config']['callback_notify_url'] = array(
    '#type'           => 'textfield',
    '#title'            => t('Notification URL'),
    '#description'      => t('The notification URL is used to keep your database/system actual and synchronizes with the Novalnet transaction status.'),
    '#default_value'    => (!empty($global_setting['callback_notify_url'])) ? $global_setting['callback_notify_url'] : url('', array('absolute' => TRUE)) . '?q=commerce_novalnet_callback',
  );
  $form['callback_config']['callback_mail_settings'] = array(
    '#type'             => 'fieldset',
    '#states'           => array('visible' => array('input[name="callback_config[callback_mail_send]"]' => array("checked" => TRUE))),
  );
  $form['callback_config']['callback_mail_settings']['callback_mail_from'] = array(
    '#type'             => 'textfield',
    '#title'            => t('E-mail address (From)'),
    '#description'      => t('E-mail address of the sender'),
    '#default_value'    => (!empty($callback_setting) && !empty($callback_setting['callback_mail_from'])) ? $callback_setting['callback_mail_from'] : variable_get('site_mail'),
    '#element_validate' => array('commerce_novalnet_element_validate_email'),
  );
  $form['callback_config']['callback_mail_settings']['callback_mail_to'] = array(
    '#type'             => 'textfield',
    '#title'            => t('E-mail address (To)'),
    '#description'      => t('E-mail address of the recipient.'),
    '#default_value'    => (!empty($callback_setting)) ? $callback_setting['callback_mail_settings']['callback_mail_to'] : '',
    '#element_validate' => array('commerce_novalnet_element_validate_email'),
  );
  $form['callback_config']['callback_mail_settings']['callback_mail_bcc'] = array(
    '#type'             => 'textfield',
    '#title'            => t('E-mail address (Bcc)'),
    '#description'      => t('E-mail address of the recipient for BCC.'),
    '#default_value'    => (!empty($callback_setting)) ? $callback_setting['callback_mail_settings']['callback_mail_bcc'] : '',
    '#element_validate' => array('commerce_novalnet_element_validate_email'),
  );
  $form['callback_config']['callback_mail_settings']['callback_mail_subject'] = array(
    '#type'             => 'textfield',
    '#title'            => t('E-mail subject'),
    '#default_value'    => (!empty($callback_setting)) ? $callback_setting['callback_mail_settings']['callback_mail_subject'] : t('Novalnet Callbackscript Access Report'),
  );
  $form['callback_config']['callback_mail_settings']['callback_mail_body'] = array(
    '#type'             => 'textarea',
    '#title'            => t('E-mail body'),
    '#default_value'    => (!empty($callback_setting)) ? $callback_setting['callback_mail_settings']['callback_mail_body'] : '',
  );
  $var_info = array(
    'site' => array(
      'type' => 'site',
      'label' => t('Site information'),
      'description' => t('Site-wide settings and other global information.'),
    ),
    'commerce_order' => array(
      'label' => t('Order'),
      'type' => 'commerce_order',
    ),
  );
  $form['callback_config']['callback_mail_settings']['callback_mail_message_help'] = RulesTokenEvaluator::help($var_info);
  $form['submit']  = array(
    '#type'             => 'submit',
    '#value'            => t('Configure'),
    '#attributes'       => array('id' => 'nn-global-submit'),
    '#submit'           => array('commerce_novalnet_global_config_form_submit'),
  );
  $form['#attached']['js'][] = array(
    'data' => drupal_get_path('module', 'commerce_novalnet') . '/js/commerce_novalnet.js',
    'type' => 'file',
  );

  return $form;
}

/**
 * Function for load Novalnet Administration Portal.
 *
 * @param array $form
 * @param array $form_state
 *
 * @return array
 */
function commerce_novalnet_admin_portal_form($form, &$form_state) {
  $form['admin_portal'] = array(
    '#type'        => 'fieldset',
    '#title'       => t('Please login here with Novalnet merchant credentials. Please contact us on <a href=\'mailto:support@novalnet.de\'>support@novalnet.de</a> for activating payment methods!'),
  );
  $form['admin_portal']['markup'] = array(
    '#type'        => 'markup',
    '#markup'       => '<iframe src="https://admin.novalnet.de" width="100%" height="700px"></iframe>',
  );
  return $form;
}

/**
 * Function to get the commonly used setting / values.
 *
 * @param array $form
 * @param array $settings
 */
function commerce_novalnet_payment_common_settings(&$form, $settings) {
  $form['test_mode'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Enable test mode'),
    '#description'   => t('The payment will be processed in the test mode therefore amount for this transaction will not be charged'),
    '#default_value' => isset($settings['test_mode']) ? $settings['test_mode'] : 0,
  );
  $form['order_status'] = array(
    '#type'          => 'select',
    '#title'         => t('Order completion status'),
    '#options'       => commerce_order_status_options_list(),
    '#default_value' => isset($settings['order_status']) ? $settings['order_status'] : 'pending',
    '#weight'        => 7,
  );
  $form['enduser_info'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Notification for the buyer'),
    '#description'   => t('The entered text will be displayed on the checkout page'),
    '#default_value' => isset($settings['enduser_info']) ? $settings['enduser_info'] : '',
    '#weight'        => 8,
  );
  $form['reference1']   = array(
    '#type'          => 'textfield',
    '#title'         => t('Transaction reference 1'),
    '#description'   => t('This reference will appear in your bank account statement'),
    '#default_value' => isset($settings['reference1']) ? $settings['reference1'] : '',
    '#weight'        => 9,
  );
  $form['reference2']   = array(
    '#type'           => 'textfield',
    '#title'          => t('Transaction reference 2'),
    '#description'   => t('This reference will appear in your bank account statement'),
    '#default_value' => isset($settings['reference2']) ? $settings['reference2'] : '',
    '#weight'        => 10,
  );
}

/**
 * Function for adding additional settings for Invoice & Prepayment payment methods.
 *
 * @param array $form
 * @param array $settings
 */
function commerce_novalnet_invoice_prepayment_additional_settings(&$form, $settings) {
  // Core function for get the available status.
  $order_status   = commerce_order_status_options_list();
  $form['payment_references'] = array(
    '#type'             => 'fieldset',
    '#element_validate' => array('commerce_novalnet_payment_ref_validation'),
  );
  $form['payment_references']['payment_ref_1'] = array(
    '#type'           => 'checkbox',
    '#title'          => t('Payment Reference 1 (Novalnet Invoice Reference)'),
    '#default_value'  => isset($settings['payment_references']['payment_ref_1']) ? $settings['payment_references']['payment_ref_1'] : 1,
  );
  $form['payment_references']['payment_ref_2'] = array(
    '#type'           => 'checkbox',
    '#title'          => t('Payment Reference 2 (TID)'),
    '#default_value'  => isset($settings['payment_references']['payment_ref_2']) ? $settings['payment_references']['payment_ref_2'] : 1,
  );
  $form['payment_references']['payment_ref_3'] = array(
    '#type'           => 'checkbox',
    '#title'          => t('Payment Reference 3 (Order No)'),
    '#default_value'  => isset($settings['payment_references']['payment_ref_3']) ? $settings['payment_references']['payment_ref_3'] : 1,
  );

  $form['callback_status'] = array(
    '#type'          => 'select',
    '#title'         => t('Callback order status'),
    '#options'       => $order_status,
    '#default_value' => isset($settings['callback_status']) ? $settings['callback_status'] : 'completed',
  );

}

/**
 * Function for adding date of birth field for guarantee payments.
 *
 * @param array $form
 * @param array $settings
 * @param object $order
 */
function commerce_novalnet_dob_field(&$form, $settings, $order) {
  $billing_wrapper   = entity_metadata_wrapper('commerce_order', $order);
  $billing_details  = $billing_wrapper->commerce_customer_billing->commerce_customer_address->value();

  // Add date of birth field if guarantee payment enabled, billing address belong to the country AT/DE/CH and order currency only for Euro.
  if (!empty($settings['guarantee_payment']['enable_guarantee']) && in_array($billing_details['country'], array('AT', 'DE', 'CH')) && $order->commerce_order_total['und'][0]['currency_code'] == 'EUR') {

    if (module_exists('date_popup')) {
      $form['nn_birth_date'] = array(
        '#title'              => t('Date of birth'),
        '#type'               => 'date_popup',
        '#date_year_range'    => '1900:0',
        '#date_format'        => 'Y-m-d',
        '#attributes'         => array('readonly' => 'readonly'),
        '#default_value'      => date('Y-m-d'),
        '#datepicker_options' => array('maxDate' => 0),
        '#prefix'             => '<div id="datepicker-dob">',
        '#suffix'             => '</div>',
        '#required'           => TRUE,
        '#element_validate' => array('commerce_novalnet_element_validate_required'),
      );
    }
    else {
      $form['nn_birth_date_unformat'] = array(
        '#title'            => t('Date of birth'),
        '#type'             => 'date',
        '#default_value' => array(
          'month' => format_date(time(), 'custom', 'n'),
          'day' => format_date(time(), 'custom', 'j'),
          'year' => format_date(time(), 'custom', 'Y'),
        ),
        '#element_validate' => array('commerce_novalnet_element_validate_dob'),
        '#required'         => TRUE,
      );
    }
  }
}

/**
 * Function for add fields fop guarantee payment settings.
 *
 * @param array $form
 * @param array $settings
 */
function commerce_novalnet_guarantee_payment_settings(&$form, $settings) {
  $form['guarantee_payment'] = array(
    '#type'             => 'fieldset',
    '#title'            => t('Payment guarantee configuration'),
    '#weight'           => 20,
    '#collapsible' => TRUE,
    '#collapsed'   => TRUE,
  );
  $form['guarantee_payment']['hint'] = array(
    '#type'             => 'markup',
    '#markup'           => '<label>' . t('Basic requirements for payment guarantee') . '</label>
                            <ul>
                            <li>' . t('Allowed countries: AT, DE, CH') . '</li>
                            <li>' . t('Allowed currency: EUR') . '</li>
                            <li>' . t('Minimum amount of order >= 20,00 EUR') . '</li>
                            <li>' . t('Maximum amount of order <= 5.000,00 EUR') . '</li>
                            <li>' . t('Minimum age of end customer >= 18 Years') . '</li>
                            <li>' . t('The billing address must be the same as the shipping address') . '</li>
                            <li>' . t('Gift certificates/vouchers are not allowed') . '</li>
                            </ul>',
  );
  $form['guarantee_payment']['enable_guarantee'] = array(
    '#type'           => 'checkbox',
    '#title'          => t('Enable payment guarantee'),
    '#default_value'  => isset($settings['guarantee_payment']['enable_guarantee']) ? $settings['guarantee_payment']['enable_guarantee'] : 0,
  );
  $form['guarantee_payment']['min_amount'] = array(
    '#type'             => 'textfield',
    '#title'            => t('Minimum order amount (in minimum unit of currency. E.g. enter 100 which is equal to 1.00)'),
    '#default_value'    => isset($settings['guarantee_payment']['min_amount']) ? $settings['guarantee_payment']['min_amount'] : '',
    '#description'    => t('This setting will override the default setting made in the minimum order amount. Note that amount should be in the range of 20,00 EUR - 5.000,00 EUR'),
    '#element_validate' => array('commerce_novalnet_element_validate_guarantee_amount'),
  );
  $form['guarantee_payment']['max_amount'] = array(
    '#type'             => 'textfield',
    '#title'            => t('Maximum order amount (in minimum unit of currency. E.g. enter 100 which is equal to 1.00)'),
    '#default_value'    => isset($settings['guarantee_payment']['max_amount']) ? $settings['guarantee_payment']['max_amount'] : '',
    '#description'    => t('This setting will override the default setting made in the maximum order amount. Note that amount should be greater than minimum order amount, but not more than 5.000,00 EUR'),
    '#element_validate' => array('commerce_novalnet_element_validate_guarantee_amount'),
  );

  $form['guarantee_payment']['force_guarantee'] = array(
    '#type'           => 'checkbox',
    '#title'          => t('Force Non-Guarantee payment'),
    '#description'    => t('If the payment guarantee is activated (True), but the above mentioned requirements are not met, the payment should be processed as non-guarantee payment.'),
    '#default_value'  => isset($settings['guarantee_payment']['force_guarantee']) ? $settings['guarantee_payment']['force_guarantee'] : 1,
  );
}

/**
 * Function for adding fraud module settings field in back-end.
 *
 * @param array $form
 * @param array $settings
 *
 * @return array
 */
function commerce_novalnet_fraud_settings(&$form, $settings) {
  $form['pin_method'] = array(
    '#type'              => 'select',
    '#title'             => t('Enable fraud prevention'),
    '#description'       => t('To authenticate the buyer for a transaction, the PIN will be automatically generated and sent to the buyer. This service is only available for customers from DE, AT, CH.'),
    '#options'           => array(
      0          => t('None'),
      'callback' => t('PIN by callback'),
      'sms'      => t('PIN by SMS'),
    ),
    '#default_value'     => isset($settings['pin_method']) ? $settings['pin_method'] : '',
  );
  $form['pin_amount'] = array(
    '#type'             => 'textfield',
    '#title'            => t('Minimum value of goods for the fraud module (in minimum unit of currency. E.g. enter 100 which is equal to 1.00)'),
    '#description'      => t('Enter the minimum value of goods from which the fraud module should be activated.'),
    '#default_value'    => isset($settings['pin_amount']) ? $settings['pin_amount'] : '',
    '#element_validate' => array('commerce_novalnet_element_validate_number'),
    '#states'           => array(
      'invisible' => array('select[id="edit-parameter-payment-method-settings-payment-method-settings-pin-method"]' => array('value' => 0)),
    ),
  );
  return $form;
}

/**
 * Function for assign merchant details on hidden fields.
 *
 * @param array $form
 * @param array $global_setting
 */
function commerce_novalnet_merchant_hidden_details(&$form, $global_setting) {
  commerce_novalnet_check_affiliate_order($global_setting);
  $form['nn_auth_code'] = array(
    '#type'             => 'hidden',
    '#default_value'    => $global_setting['auth_code'],
    '#attributes'       => array('id' => 'nn_auth_code'),
  );
  $form['nn_vendor_id'] = array(
    '#type'             => 'hidden',
    '#default_value'    => $global_setting['vendor_id'],
    '#attributes'       => array('id' => 'nn_vendor_id'),
  );
  $form['nn_unique_id'] = array(
    '#type'             => 'hidden',
    '#default_value'    => commerce_novalnet_get_random_string(),
    '#attributes'       => array('id' => 'nn_unique_id'),
  );
  $form['nn_form_loading'] = array(
    '#type'    => 'markup',
    '#markup'  => '<div id="loading-img"><img src="' . NOVALNET_IMAGES_PATH . 'commerce_novalnet_loader.gif"></div>',
  );
}

/**
 * Function for adding form fields for fraud module in front-end.
 *
 * @param array $form
 * @param array $settings
 * @param object $order
 * @param array $payment_method
 */
function commerce_novalnet_fraud_prevention_form(&$form, $settings, $order, $payment_method) {

  $billing_wrapper  = entity_metadata_wrapper('commerce_order', $order);
  $billing_details  = $billing_wrapper->commerce_customer_billing->commerce_customer_address->value();

  // If any one of the fraud module is enabled and the order total is greater then the minimum value of fraud module.
  if (!empty($settings['pin_method']) && $order->commerce_order_total['und'][0]['amount'] >= (int) $settings['pin_amount'] && in_array($billing_details['country'], array('AT', 'DE', 'CH'))) {

    if (!isset($order->data['payment_method'])) {
      goto fraud_input;
    }

    if (isset($_SESSION['novalnet'][$payment_method]['tid'])) {

      if (((isset($settings['shopping_type']) && $settings['shopping_type'] != 'zero_amount') || $_SESSION['novalnet'][$payment_method]['recurring']) && $_SESSION['novalnet'][$payment_method]['amount'] != $order->commerce_order_total['und'][0]['amount']) {
        commerce_novalnet_session();
        drupal_set_message(t('The order amount has been changed, please proceed with the new order'), 'warning');
        goto fraud_input;
      }
      $form['nn_fraud_check'] = array(
        '#type'          => 'hidden',
        '#default_value' => 1,
      );
      $form['nn_fraud_pin'] = array(
        '#type'          => 'textfield',
        '#title'         => t('Transaction PIN'),
        '#size'          => 5,
        '#attributes'    => array('autocomplete' => 'off'),
        '#states' => array(
          'required' => array(':input[id=forget_pin]' => array('value' => 0)),
        ),
        '#weight'        => 9,
        '#element_validate' => array('commerce_novalnet_element_validate_pin'),
      );
      $form['nn_fraud_resend'] = array(
        '#type'          => 'checkbox',
        '#title'         => t('Forgot your PIN?'),
        '#attributes'    => array('id' => 'forget_pin'),
        '#weight'        => 10,
      );
    }
    else {
      fraud_input:
      $title = ($settings['pin_method'] == 'sms') ? t('Mobile number') : t('Telephone number');
      $form['nn_fraud_data'] = array(
        '#type'          => 'textfield',
        '#title'         => filter_xss($title) . ' <span title="' . t('This field is required.') . '" class="form-required">*</span>',
        '#size'          => 27,
        '#weight'        => 10,
        '#attributes'    => array('autocomplete' => 'off'),
        '#prefix'        => '<span class="messages nn_fraud_data">',
        '#suffix'        => '</span>',
        '#default_value' => '',
        '#element_validate' => array('commerce_novalnet_element_validate_fraud_data'),
      );
    }
  }
}

/**
 * Implement hook_form_alter().
 *
 * @param array $form
 * @param array $form_state
 * @param string $form_id
 */
function commerce_novalnet_form_alter(&$form, $form_state, $form_id) {
  switch ($form_id) {
    // If the form id is back-end oder edit form.
    case 'commerce_order_ui_order_form':
      $order = $form_state['commerce_order'];
      $form['#entity'] = isset($form['#entity']) ? $form['#entity'] : $order;
      if (!empty($order->data['payment_method'])
      && preg_match('/commerce_novalnet/', $order->data['payment_method'])
      // If the order has completed by using Novalnet payment.
      && isset($order->data['tid']) && $trans_details = commerce_novalnet_order_merchant_details($order->order_id, $order->data['tid'])) {
        // If the transaction not canceled.
        if (!in_array($trans_details['status'], array(103, 105))) {
          $form['commerce_novalnet_datas']['transaction_status'] = array(
            '#type'            => 'hidden',
            '#default_value'   => $trans_details['status'],
            '#attributes'    => array('id' => 'transaction_status'),
          );
          $form['#attached']['js'] = array(
            drupal_get_path('module', 'commerce_novalnet') . '/js/commerce_novalnet.js',
          );
          $form['commerce_novalnet_actions'] = array(
            '#type'          => 'fieldset',
            '#title'         => t('Novalnet Transactions Overview'),
            '#collapsible'   => TRUE,
            '#collapsed'     => TRUE,
            '#group'         => 'additional_settings',
          );

          // If the transaction not confirmed / Canceled or transaction was on-hold.
          if (in_array($trans_details['status'], array(85, 91, 98, 99))) {
            if ($trans_details['payment_key'] == 37) {
              commerce_novalnet_transaction_amount_update_form($form, $trans_details['org_total']);
            }
            // Call this function for load transaction update form.
            commerce_novalnet_transaction_status_update_form($form);
          }
          // If the transaction was succeeded.
          elseif (!empty($trans_details['booked'])) {
            if (in_array($trans_details['payment_key'], array(27, 41)) || $trans_details['paid_amount'] > 0) {
              // Call this function for load transaction amount refund form.
              commerce_novalnet_transaction_amount_refund_form($form, $trans_details['org_total'], $trans_details['payment_key']);
            }
            if ($trans_details['payment_key'] == 27 && $trans_details['paid_amount'] < $trans_details['org_total']) {

              commerce_novalnet_transaction_amount_update_form($form, $trans_details['org_total'], TRUE);
              $payment_details = unserialize($trans_details['payment_details']);

              if (module_exists('date_popup')) {
                $form['commerce_novalnet_actions']['amount_update_action']['due_date'] = array(
                  '#type'               => 'date_popup',
                  '#title'              => t('Transaction due date'),
                  '#date_year_range'    => date('Y') . ':3',
                  '#date_format'        => 'Y-m-d',
                  '#default_value'      => $payment_details['due_date'],
                  '#attributes'         => array('readonly' => 'readonly'),
                  '#datepicker_options' => array('minDate' => 0),
                );
              }
              else {
                $due_date = strtotime($payment_details['due_date']);
                $form['commerce_novalnet_actions']['amount_update_action']['due_date_unformated'] = array(
                  '#type'             => 'date',
                  '#title'            => t('Transaction due date'),
                  '#default_value'    => array(
                    'month' => format_date($due_date, 'custom', 'n'),
                    'day'   => format_date($due_date, 'custom', 'j'),
                    'year'  => format_date($due_date, 'custom', 'Y'),
                  ),
                  '#element_validate' => array('commerce_novalnet_element_validate_duedate'),
                );
              }
              $form['commerce_novalnet_actions']['amount_update_action']['old_due_date'] = array(
                '#type'               => 'hidden',
                '#attributes'    => array('id' => 'old_due_date'),
                '#default_value'      => $payment_details['due_date'],
              );
            }
          }
          else {
            commerce_novalnet_transaction_zero_amount_booking_form($form, $trans_details['org_total']);
          }
          $form_state['redirect'] = 'admin/commerce/orders/' . $order->order_id . '/edit';
        }
      }
      break;

    // If the form is front-end subscription cancel form.
    case 'commerce_recurring_ui_cancel_renewals_form':
      $recurring_info = $form_state['build_info']['args'][1];
      $order_id = $recurring_info->commerce_recurring_order['und'][0]['target_id'];
      $order = commerce_order_load($order_id);
      if (!empty($order->data['payment_method'])
      && preg_match('/commerce_novalnet/', $order->data['payment_method'])
      && $order->data['tid']) {

        $options = array(
          ''                                   => t('Select'),
          t('Product is costly')               => t('Product is costly'),
          t('Cheating')                        => t('Cheating'),
          t('Partner interfered')              => t('Partner interfered'),
          t('Financial problem')               => t('Financial problem'),
          t('Content does not match my likes') => t('Content does not match my likes'),
          t('Content is not enough')           => t('Content is not enough'),
          t('Interested only for a trial')     => t('Interested only for a trial'),
          t('Page is very slow')               => t('Page is very slow'),
          t('Satisfied customer')              => t('Satisfied customer'),
          t('Logging in problems')             => t('Logging in problems'),
          t('Other')                           => t('Other'),
        );

        $form['novalnet_field'] = array(
          '#type'       => 'fieldset',
        );

        $form['novalnet_field']['recurring_info'] = array(
          '#type'       => 'markup',
          '#markup'     => '<div>' . t('Registration Date & Time :') . date(' d/m/Y - H:i', $recurring_info->start_date) . '</div>
                          <div>' . t('Next recurring Date & Time :') . date(' d/m/Y - H:i', $recurring_info->due_date) . '</div>',
        );

        $form['novalnet_field']['cancel_options'] = array(
          '#type'       => 'select',
          '#title'      => t('Please select the reason of subscription cancellation'),
          '#options'    => $options,
          '#required'   => TRUE,
        );

        $form['#submit'][] = 'commerce_novalnet_recurring_cancel_form_submit';
      }
      break;

    case 'commerce_recurring_edit_product_form':
      $order_id = $form['#entity']->commerce_recurring_order['und'][0]['target_id'];
      $order = commerce_order_load($order_id);
      if (isset($order->data['subs'])) {
        if (isset($order->data['subs']['cancel'])) {
          drupal_set_message(t('The subscription @id was cancelled, you will not be charged again for this subscription.', array('@id' => $form['#entity']->id)), 'warning');
          return TRUE;
        }
        $min_date = (int) (($form['#entity']->due_date - time()) / 86400) + 1;
        $form['due_date']['#datepicker_options'] = array('minDate' => +$min_date . 'D');
        $form['due_date']['#attributes']         = array('readonly' => 'readonly');
        $options = $form['status']['#options'];
        $options[2] = t('Cancel');
        $form['status']['#options'] = $options;

        $cancel_options = array(
          ''                                   => t('Select'),
          t('Product is costly')               => t('Product is costly'),
          t('Cheating')                        => t('Cheating'),
          t('Partner interfered')              => t('Partner interfered'),
          t('Financial problem')               => t('Financial problem'),
          t('Content does not match my likes') => t('Content does not match my likes'),
          t('Content is not enough')           => t('Content is not enough'),
          t('Interested only for a trial')     => t('Interested only for a trial'),
          t('Page is very slow')               => t('Page is very slow'),
          t('Satisfied customer')              => t('Satisfied customer'),
          t('Logging in problems')             => t('Logging in problems'),
          t('Other')                           => t('Other'),
        );
        $form['cancel_options'] = array(
          '#type'             => 'select',
          '#title'            => t('Please select the reason of subscription cancellation'),
          '#options'          => $cancel_options,
          '#weight'           => 40,
          '#element_validate' => array('commerce_novalnet_element_validate_cancel_options'),
        );
        $form['#submit'][] = 'commerce_novalnet_recurring_update_submit';
        $form['#attached']['js'] = array(
          drupal_get_path('module', 'commerce_novalnet') . '/js/commerce_novalnet.js',
        );
      }

      break;
  }
}

/**
 * Function for display transaction amount update form.
 *
 * @param array $form
 * @param int $org_amount
 * @param bool $due_date
 */
function commerce_novalnet_transaction_amount_update_form(&$form, $org_amount, $due_date = FALSE) {

  $title = ($due_date) ? t('Change the amount / due date') : t('Amount Update');
  $form['novalnet_amount_update_process'] = array(
    '#type'         => 'hidden',
    '#attributes'    => array('id' => 'novalnet_amount_update_process'),
  );
  $form['commerce_novalnet_actions']['amount_update_action'] = array(
    '#type'          => 'fieldset',
    '#title'         => filter_xss($title),
    '#collapsible'   => TRUE,
    '#collapsed'     => TRUE,
  );
  $form['commerce_novalnet_actions']['amount_update_action']['amount_update'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Transaction Amount'),
    '#size'          => 20,
    '#default_value' => $org_amount,
  );
  $form['commerce_novalnet_actions']['amount_update_action']['old_amount'] = array(
    '#type'          => 'hidden',
    '#attributes'    => array('id' => 'old_amount'),
    '#default_value' => $org_amount,
  );
  $form['actions']['submit']['#submit'][] = 'commerce_novalnet_transaction_amount_update';
}

/**
 * Function for display transaction status update form.
 *
 * @param array $form
 */
function commerce_novalnet_transaction_status_update_form(&$form) {
  $form['novalnet_change_status_process'] = array(
    '#type'         => 'hidden',
    '#attributes'    => array('id' => 'novalnet_change_status_process'),
  );
  $form['commerce_novalnet_actions']['change_status_action'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Manage transaction process'),
    '#collapsible'   => TRUE,
    '#collapsed'     => FALSE,
    '#weight'        => -1,
  );
  $form['commerce_novalnet_actions']['change_status_action']['change_status'] = array(
    '#type'          => 'select',
    '#title'         => '',
    '#options'       => array('' => t('Select'), '100' => t('Confirm'), '103' => t('Cancel')),
  );
  $form['actions']['submit']['#submit'][] = 'commerce_novalnet_transaction_status_update';
}

/**
 * Function for display transaction amount update form.
 *
 * @param array $form
 * @param array $refund_amount
 * @param int $payment_key
 */
function commerce_novalnet_transaction_amount_refund_form(&$form, $refund_amount, $payment_key) {
  $currency = $form['#entity']->commerce_order_total['und'][0]['currency_code'];
  $form['commerce_novalnet_actions']['refund_action'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Refund Process'),
    '#collapsible'   => TRUE,
    '#collapsed'     => FALSE,
    '#weight'        => -1,
  );
  $form['novalnet_amount_refund_process'] = array(
    '#type'         => 'hidden',
    '#attributes'    => array('id' => 'novalnet_amount_refund_process'),
  );
  $form['commerce_novalnet_actions']['refund_action']['refund_amount'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Please enter the refund amount (in minimum unit of currency. E.g. enter 100 which is equal to 1.00)'),
    '#size'          => 20,
    '#description'   => t('Amount : @amount', array('@amount' => commerce_currency_format($refund_amount, $currency))),
    '#default_value' => 0,
  );

  $update_on = $form['#entity']->data['transaction_details'][0]['update_on'];
  if (time() > strtotime('+1 day', $update_on)) {
    $form['commerce_novalnet_actions']['refund_action']['refund_ref'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Refund reference'),
      '#default_value' => '',
    );
  }

  if (in_array($payment_key, array(27, 33, 49, 69, 50))) {
    $form['commerce_novalnet_actions']['refund_action']['refund_options'] = array(
      '#type'          => 'select',
      '#title'         => t('Select the refund option'),
      '#options'       => array('' => t('None'), 'SEPA' => t('Direct Debit SEPA')),
      '#attributes'    => array('id' => 'refund_option'),
      '#default_value' => '',
      '#element_validate' => array('commerce_novalnet_element_validate_refund_options'),
    );
    $form['commerce_novalnet_actions']['refund_action']['refund_ac_holder'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Account holder'),
      '#attributes'     => array('autocomplete' => 'off'),
      '#default_value' => '',
      '#states'        => array(
        'visible' => array(':input[id="refund_option"]' => array('value' => 'SEPA')),
      ),
    );
    $form['commerce_novalnet_actions']['refund_action']['refund_ac_iban'] = array(
      '#type'          => 'textfield',
      '#title'         => t('IBAN'),
      '#attributes'     => array('autocomplete' => 'off'),
      '#default_value' => '',
      '#states'        => array(
        'visible' => array(':input[id="refund_option"]' => array('value' => 'SEPA')),
      ),
    );
    $form['commerce_novalnet_actions']['refund_action']['refund_ac_bic'] = array(
      '#type'          => 'textfield',
      '#title'         => t('BIC'),
      '#attributes'     => array('autocomplete' => 'off'),
      '#default_value' => '',
      '#states'        => array(
        'visible' => array(':input[id="refund_option"]' => array('value' => 'SEPA')),
      ),
    );
  }

  $form['actions']['submit']['#submit'][] = 'commerce_novalnet_transaction_amount_refund';
}

/**
 * Function for display transaction booking form.
 *
 * @param array $form
 * @param int $org_total
 */
function commerce_novalnet_transaction_zero_amount_booking_form(&$form, $org_total) {
  $currency = $form['#entity']->commerce_order_total['und'][0]['currency_code'];
  $form['commerce_novalnet_actions']['manage_transaction'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Book transaction'),
    '#collapsible'   => TRUE,
    '#collapsed'     => FALSE,
  );
  $form['commerce_novalnet_actions']['manage_transaction']['book_amount'] = array(
    '#type'          => 'textfield',
    '#size'          => 20,
    '#title'         => t('Transaction booking amount (in minimum unit of currency. E.g. enter 100 which is equal to 1.00)'),
    '#description'   => t('Order total : @amount', array('@amount' => commerce_currency_format($org_total, $currency))),
    '#default_value' => 0,
  );
  $form['actions']['submit']['#submit'][] = 'commerce_novalnet_transaction_zero_amount_booking';
}

/**
 * Function for set guarantee payment error.
 *
 * @param none
 */
function commerce_novalnet_guarantee_error_form() {
  return array(
    '#type'   => 'markup',
    '#weight' => '50',
    '#markup' => '<div class="messages warning">' . t("The payment cannot be processed, because the basic requirements haven’t been met.") . '</div>',
  );
}
