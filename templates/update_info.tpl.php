<?php
    drupal_add_js('misc/collapse.js');
    drupal_add_js('misc/form.js');
    drupal_add_js(drupal_get_path('module', 'commerce_novalnet') . '/js/commerce_novalnet.js');
    drupal_add_css(drupal_get_path('module', 'commerce_novalnet') . '/css/commerce_novalnet.css');
?>
<div class="nn_update_info">
    <div class="version_info">
        <div>
            <img style="float:right" src="<?php echo NOVALNET_IMAGES_PATH;?>novalnet-logo.jpg">
            <h3><?php echo t('Novalnet Payment Plugin V11.1.2');?></h3>
        </div>
        <div>
            <p><?php echo t('Thank you for updating to the latest version of Novalnet Payment Plugin.');?></p>
            <p><?php echo t('This version introduces some great new features and enhancements.');?></p>
            <p><?php echo t('We hope you enjoy it!');?></p>
            <p><a href="<?php echo base_path();?>/admin/commerce/config/novalnet-settings/global-config"><?php echo t('Novalnet Global Configuration »');?></a></p>
        </div>
    </div>
</div>

<fieldset>
    <legend>
        <span class="fieldset-legend"><?php echo t('Check Out What\'s New');?></span>
    </legend>
    <div class="fieldset-wrapper">
        <fieldset class="collapsible">
            <legend>
                <span class="fieldset-legend"><?php echo t('Product Activation Key');?></span>
            </legend>
            <div class="nn_info fieldset-wrapper">
                <div class="nn_row">
                <div class="left">
                    <p><?php echo t('Novalnet introduces Product Activation Key to fill entire merchant credentials automatically on entering the key into the Novalnet Global Configuration');?>
                    </p>
                </div>
                <div class="right">
                    <div class="nn_info_img">
                        <img src="<?php echo NOVALNET_IMAGES_PATH . t('update_info/en/admin_product_list.png')?>">
                    </div>
                </div>
                </div>
                <div class="nn_row">
                <div class="left">
                    <p><?php echo t('To get the Product Activation Key, please go to Novalnet admin portal - PROJECTS: Project Information - Shop Parameters: API Signature (Product activation key).');?>
                    </p>
                </div>
                <div class="right">
                    <div class="nn_info_img">
                        <img src="<?php echo NOVALNET_IMAGES_PATH . t('update_info/en/admin_shop_params.png')?>">
                    </div>
                </div>
                </div>
            </div>
        </fieldset>
        <fieldset class="collapsible">
            <legend>
                <span class="fieldset-legend"><?php echo t('IP Address Configuration');?></span>
            </legend>
            <div class="nn_info fieldset-wrapper">
                <div class="nn_row">
                <div class="left">
                    <div class="nn_info_img">
                        <img src="<?php echo NOVALNET_IMAGES_PATH . t('update_info/en/admin_product_list.png')?>">
                    </div>
                </div>
                <div class="right">
                    <p><?php echo t('For all API access (Auto configuration with Product Activation Key, loading Credit Card iframe, Transaction API access, Transaction status enquiry, and update), it is required to configure a server IP address in Novalnet administration portal.');?>
                    </p>
                </div>
                </div>
                <div class="nn_row">
                <div class="left">
                    <div class="nn_info_img">
                        <img src="<?php echo NOVALNET_IMAGES_PATH . t('update_info/en/admin_project_ip_config.png')?>">
                    </div>
                </div>
                <div class="right">
                    <p><?php echo t('To configure an IP address, please go to Novalnet admin portal - PROJECTS: Project Information - Project Overview: Payment Request IP\'s - Update Payment Request IP.');?>
                    </p>
                </div>
                </div>
            </div>
        </fieldset>
        <fieldset class="collapsible">
            <legend>
                <span class="fieldset-legend"><?php echo t('Update of Vendor Script URL');?></span>
            </legend>
            <div class="nn_info fieldset-wrapper">
                <div class="nn_row">
                <div class="left">
                    <p><?php echo t('Vendor script URL is required to keep the merchant’s database/system up-to-date and synchronized with Novalnet transaction status. It is mandatory to configure the Vendor Script URL in Novalnet administration portal.<br>Novalnet system (via asynchronous) will transmit the information on each transaction and its status to the merchant’s system.');?>
                    </p>
                </div>
                <div class="right">
                    <div class="nn_info_img">
                        <img src="<?php echo NOVALNET_IMAGES_PATH . t('update_info/en/admin_product_list.png')?>">
                    </div>
                </div>
                </div>
                <div class="nn_row">
                <div class="left">
                    <p><?php echo t('To configure Vendor Script URL, please go to Novalnet admin portal - PROJECTS: Project Information - Project Overview - Vendor script URL.');?>
                    </p>
                </div>
                <div class="right">
                    <div class="nn_info_img">
                        <img src="<?php echo NOVALNET_IMAGES_PATH . t('update_info/en/admin_project_vendor_script.png')?>">
                    </div>
                </div>
                </div>
            </div>
        </fieldset>
        <fieldset class="collapsible">
            <legend>
                <span class="fieldset-legend"><?php echo t('PAYPAL');?></span>
            </legend>
            <div class="nn_info fieldset-wrapper">
                <div class="nn_row">
                <div class="left">
                    <div class="nn_info_img">
                        <img src="<?php echo NOVALNET_IMAGES_PATH . t('update_info/en/admin_payment_list.png')?>">
                    </div>
                </div>
                <div class="right">
                    <p><?php echo t('To proceed transaction in PayPal payment, it is required to configure PayPal API details in Novalnet administration portal.');?>
                    </p>
                </div>
                </div>
                <div class="nn_row">
                <div class="left">
                    <div class="nn_info_img">
                        <img src="<?php echo NOVALNET_IMAGES_PATH . t('update_info/en/admin_paypal_confiq.png')?>">
                    </div>
                </div>
                <div class="right">
                    <p><?php echo t('To configure Paypal API details, please go to Novalnet admin portal - PROJECTS: Project Information - Payment Methods: Paypal - Configure.');?>
                    </p>
                </div>
                </div>
            </div>
        </fieldset>
        <fieldset class="collapsible">
            <legend>
                <span class="fieldset-legend"><?php echo t('But wait, there\'s more!');?></span>
            </legend>
            <div class="nn_info fieldset-wrapper">
                <h3><?php echo t('One Click Shopping');?></h3>
                <p><?php echo t('Want your customers to make an order with a single click?');?></p>
                <p><?php echo t('With Novalnet payment plugin, they can! This feature can make the end customer to make order more conveniently with saved account/card details.');?></p>
                <h3><?php echo t('Zero Amount Booking');?></h3>
                <p><?php echo t('Zero amount booking feature makes it possible for the merchant to sell variable amount product in the shop. Order will be processed with Zero amount initially, then the merchant can book the order amount later to complete the transaction.');?></p>
                <h3><?php echo t('Credit Card Responsive Iframe');?></h3>
                <p><?php echo t('Now, we have updated the Credit Card with the most dynamic features. With the little bit of code, we have made the Credit Card iframe content responsive friendly.');?></p>
                <p><?php echo t('The merchant can customize the CSS settings of the Credit Card iframe form.');?></p>
            </div>
        </fieldset>
    </div>
</fieldset>
<div class="nnoverlay">
    <div class="nnoverlay-bg" ></div>
    <div class="nnoverlay-content"><span class="nnoverlay-content close"></span>
    <img src="<?php echo NOVALNET_IMAGES_PATH . t('update_info/en/admin_product_list.png')?>">
    </div>
</div>
