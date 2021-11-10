/**
 * @file
 * Script for commerce_novalnet_paypal module.
 */

(function ($) {
  Drupal.behaviors.commerceNovalnetPayPal = {
    attach: function (context, settings) {
      if ($('#paypal_mask_form', context).size() > 0) {
        $('#paypal_normal_form').hide();
        $('#change_mask_paypal').live('click', function (event) {
          event.stopImmediatePropagation();
          event.preventDefault();
          if ($('#from_paypal_mask').val() == 1) {
            $('#paypal_mask_form').hide();
            $('#paypal_normal_form').show();
            $('#from_paypal_mask').val(0);
            $('#change_mask_paypal').html(Drupal.t('Given PayPal account details'));
          }
          else {
            $('#paypal_mask_form').show();
            $('#paypal_normal_form').hide();
            $('#from_paypal_mask').val(1);
            $('#change_mask_paypal').html(Drupal.t('Proceed with new PayPal account details'));
          }
        });
      }
    }
  };
})(jQuery);
