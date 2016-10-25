{
    "job":
    {
        "command": "backup", 
        "job_data": 
        {
            "backup_method": "mysqldump", 
            "backupdir": "/dbbackup01", 
            "cc_storage": "0", 
            "hostname": "192.168.33.121"
        }
    },
    "operation": "schedule", 
    "schedule" : "0 15 * * 3"
}
