/**
* @file
* Script for commerce_novalnet_sepa module.
*/

(function ($) {
    Drupal.behaviors.commerceNovalnetSEPA = {
      attach: function (context, settings) {
        if ($('input#nn_sepa_hash').size() > 0) {
          var form = $('input#nn_sepa_hash', context).parents('form');
          reset_sepa_fields(form);

          if ($('#from_sepa_mask', form).val() != undefined && $('#from_sepa_mask', form).val() == 1) {
            show_sepa_mask(form);
          }
          else {
            hide_sepa_mask(form);
          }

          $('#change_mask_sepa', form).click(function (event) {
            event.stopImmediatePropagation();
            event.preventDefault();
            $('.nnsepa_ibanconf').removeAttr('checked');
            if ($('#from_sepa_mask', form).val() == 1) {
              hide_sepa_mask(form);
            }
            else {
              show_sepa_mask(form)
            }
          });

          if (Drupal.settings.nn_sepa_refill_hash != '' &&  $('input#nn_sepa_refill_inprocess').val() != 1) {
            sepa_refil()
          }

          $('#nnsepa_bcode, #nnsepa_acno, #nnsepa_holder', form).keypress(function (event) {
            var keycode = ('which' in event) ? event.which : event.keyCode;
            var reg = /[\[\]\/\\#,+@!^()$~%'"=:;<>{}\_\|\d*?`§]/g;
            if (this.id != 'nnsepa_holder') {
              reg = /[\[\]\/\\#,+@!^()$~%-.&'"=:;<>{}\_\|\s*?`]/g;
            }
            return !reg.test(String.fromCharCode(keycode));
          });

          $('#nnsepa_bcode, #nnsepa_acno', form).keyup(function (event) {
            this.value = this.value.toUpperCase();
          });

          $.each([ 'nnsepa_holder', 'nnsepa_country', 'nnsepa_acno', 'nnsepa_bcode'], function (i, keyVal) {
            $('#' + keyVal).live('change', function () {
              $('#nn_sepa_hash').val('');
              $('#frm_iban').html('');
              $('#frm_bic').html('');
              $('#nnsepa_iban_div').hide();
              $('#nnsepa_bic_div').hide();
              $('#over_bic').html('');
              $('#over_iban').html('');
              $('.nnsepa_ibanconf').attr('checked', false);
            });
          });

          $('.nnsepa_ibanconf').live('click', function (event) {
            if (!this.checked) {
              $(".nnsepa_ibanconf", form).removeAttr('disabled');
              return true;
            }
            if (($('#from_sepa_mask', form).val() != undefined && $('#from_sepa_mask', form).val() == 1) || $('#nn_sepa_hash', form).val() != '') {
              return true;
            }
            $(".nnsepa_ibanconf").attr('disabled','disabled');
              event.stopPropagation();
              event.stopImmediatePropagation();
              return novalnet_get_sepa_hash(form);
          });

          $('#edit-back').live('click', function () {
            $(this).addClass('nn-go-back');
          });
          $(form).once('commerceNovalnetSEPA', function () {
            $('input[type=submit], #edit-submit', form).click(function (event) {
              if (this.id == 'edit-back') {
                return true;
              }
              if ($("input[name='commerce_payment[payment_method]']:checked").attr('value') == "commerce_novalnet_sepa|commerce_payment_commerce_novalnet_sepa" || $("input[name='commerce_payment[payment_method]']").size() == 0) {
                if ($('input#nn_sepa_hash').size() > 0) {
                  if ($('#from_sepa_mask', form).val() != undefined && $('#from_sepa_mask', form).val() == 1) {
                    return true;
                  }
                  if (!$('.nnsepa_ibanconf').is(':checked')) {
                    alert(Drupal.t('Please accept the SEPA direct debit mandate'));
                    event.stopImmediatePropagation();
                    event.preventDefault();
                    return false;
                  }
                }
              }
              return true;
            });
          });
        }
    }
  };

  function show_sepa_mask(form) {
    $('#change_mask_sepa', form).html(Drupal.t('Enter new account details'));
    $('#nnsepa_mask_form', form).show();
    $('#nnsepa_normal_form, .nn_fraud_data', form).hide();
    $('#from_sepa_mask', form).val(1)
  }
  function hide_sepa_mask(form) {
    $('#change_mask_sepa', form).html(Drupal.t('Given account details'));
    $('#from_sepa_mask', form).val(0)
    $('#nnsepa_mask_form', form).hide();
    $('#nnsepa_normal_form, .nn_fraud_data', form).show();
  }
  function reset_sepa_fields(form) {
    $(form).removeClass('novalnet-sepa-processed');
    $('#nnsepa_acno, #nnsepa_bcode, #nn_sepa_hash', form).val('');
    $('.nnsepa_ibanconf', form).removeAttr('checked');
    $('#edit-continue', form).css('display','inline');
    $('.checkout-processing', form).css('display','none');
  }

  function novalnet_get_sepa_hash(form) {
    $(".nnsepa_ibanconf").removeAttr('disabled');
    var sepaName = $.trim($('#nnsepa_holder').val());
    var sepaCountry = $('#nnsepa_country').val();
    var sepaAcno = $('#nnsepa_acno').val().replace(/[^a-z0-9]+/gi, '');
    var sepaBcode = $('#nnsepa_bcode').val().replace(/[^a-z0-9]+/gi, '');
    sepaBbic = sepaBcode;
    if (sepaCountry == 'DE' && sepaBcode == '' && isNaN(sepaAcno)) {
      sepaBcode = '123456';
      sepaBbic = '123ABC';
    }
    regAll = /[a-zA-Z]/g;
    if (sepaName == '' || sepaCountry == '' || sepaAcno == '' || sepaAcno == 0 || sepaBcode == '' || sepaBcode == 0 || !regAll.test(sepaName)) {
      alert(Drupal.t('Your account details are invalid'));
      $("#nnsepa_ibanconf").removeAttr('disabled');
      return false;
    }

    ibanReq = {'account_holder':  sepaName , 'vendor_id' : Drupal.settings.nn_vendor_id , 'vendor_authcode' : Drupal.settings.nn_auth_code , 'bank_country' : sepaCountry , 'unique_id' : Drupal.settings.nn_unique_id , 'remote_ip' : Drupal.settings.nn_remote_ip};
    if (/^(?:[0-9]+$)/.test(sepaAcno) && /^(?:[0-9]+$)/.test(sepaBcode)) {
      ibanReq = $.extend({}, ibanReq, {'bank_account': sepaAcno , 'bank_code' : sepaBcode , 'get_iban_bic':1});
      return send_xdomain_request(ibanReq, 'ibanReq');
    }
    else {
    if (/^(?:[0-9]+$)/.test(sepaAcno) || /^(?:[0-9]+$)/.test(sepaBbic)) {
      alert(Drupal.t('Please enter valid account details!'));
      $("#nnsepa_ibanconf").removeAttr('disabled');
      return false;
    }
    hashReq = $.extend({}, ibanReq, {'bank_account': '','bank_code':'','sepa_data_approved':1, 'mandate_data_req':1,'iban':sepaAcno,'bic':sepaBcode});
      return send_xdomain_request(hashReq, 'hashReq');
    }
    return true;
  }

  function alert_error(form, errorMessage) {
    alert(errorMessage);
    reset_sepa_fields(form)
  }

  function send_xdomain_request(qryObj, reqType) {
    jQuery('#loading-img').show();
    params = jQuery.param(qryObj);
    url    = 'https://payport.novalnet.de/sepa_iban';
    if ('XDomainRequest' in window && window.XDomainRequest !== null) {
      var xdr = new XDomainRequest(); // Use Microsoft XDR.
      xdr.open('POST', url);
      xdr.onload = function () {
        return check_xdomain_response(this.responseText, qryObj, reqType);
      }
      xdr.onerror = function () {
        _result = false;
      };
      xdr.send(params)
    }
    else {
      var xmlhttp = (window.XMLHttpRequest) ? new XMLHttpRequest() : new ActiveXObject("Microsoft.XMLHTTP");
      xmlhttp.onreadystatechange = function () {
      if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
        return check_xdomain_response(xmlhttp.responseText, qryObj, reqType);
      }
    }
    xmlhttp.open("POST",url,true);
    xmlhttp.setRequestHeader("Content-type","application/x-www-form-urlencoded");
    xmlhttp.send(params);
    }
  }
  function sepa_refil() {
    $('#nn_sepa_refill_inprocess').val(1);

    var qryString = {"vendor_id" : Drupal.settings.nn_vendor_id, "vendor_authcode" : Drupal.settings.nn_auth_code, "unique_id":Drupal.settings.nn_unique_id, "sepa_data_approved":1, "mandate_data_req" :1, "sepa_hash" : Drupal.settings.nn_sepa_refill_hash}
    send_xdomain_request(qryString, 'sepa_refill', null);
  }

  function check_xdomain_response(response, qryObj, reqType){
    var result = $.parseJSON(response);
    $('#loading-img').hide();

    if (result.hash_result != 'success') {
      alert(result.hash_result);
      $('.nnsepa_ibanconf').removeAttr('checked');
      return false;
    }

    switch (reqType) {
      case 'ibanReq':
        if (result.IBAN == '' || result.BIC == '') {
          alert(Drupal.t('Please enter valid account details!'));
          $('.nnsepa_ibanconf').attr('checked', false);
          return false;
        }
        $('#frm_iban').html(result.IBAN);
        $('#frm_bic').html(result.BIC);
        $('#nnsepa_iban_div, #nnsepa_bic_div').show();
        hashReq = $.extend({}, qryObj , {'sepa_data_approved':1, 'mandate_data_req':1,'iban': result.IBAN, 'bic' :result.BIC, 'get_iban_bic':0});
        send_xdomain_request(hashReq, 'hashReq');
      break;

      case 'hashReq':
        $('#nn_sepa_hash').val(result.sepa_hash);
      break;

      case 'sepa_refill':

        $('#nn_sepa_hash').val(result.sepa_hash);
        var account_holder = String(result.hash_string).match("account_holder=(.*)&bank_code");

        try {
          $('#nnsepa_holder').val(decodeURIComponent(escape(account_holder[1])));
        }
        catch (e) {
          $('#nnsepa_holder').val(account_holder[1]);
        }

        var params = result.hash_string.split("&");
        $.each(params, function (i, keyVal) {
          temp = keyVal.split('=');

          switch (temp[0]) {

            case 'iban':
              if (temp[1] !== '') {
                $('#nnsepa_acno').val(temp[1]);
              }
            break

            case 'bic':
              if (temp[1] !== '' && temp[1] != 123456) {
                $('#nnsepa_bcode').val(temp[1]);
              }
            break

            case 'bank_country':
              $('#nnsepa_country').val(temp[1]);
            break;
          }
        });

        if ($('#frm_iban').html() != '') {
          $('#nnsepa_iban_div').show();
          $('#nnsepa_bic_div').show();
        }

      break
    }
    return true;
  }
})(jQuery);

if (typeof jQuery.fn.live == 'undefined' || !(jQuery.isFunction(jQuery.fn.live))) {
  jQuery.fn.extend({
    live: function (event, callback) {
      if (this.selector) {
        jQuery(document).on(event, this.selector, callback);
      }
    }
  });
}