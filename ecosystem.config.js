module.exports = {
    apps : [{
        name       : 'it100mqtt',
        script     : 'it100mqtt',
        args       : 'settings.cfg',
        error_file : 'err.log',
        out_file   : 'out.log',
        merge_logs : true,
        "log_date_format" : "YYYY-MM-DD HH:mm",
    }],
};
