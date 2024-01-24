Running local development server: 
 - `python3 app.py`
 - Open browser on [http://localhost:8050](http://localhost:8050) or replace `localhost` with local IP of the computer. 

Running proper app server: 
 - `gunicorn app:server -b :8000`
 - Open browser on [http://<IP>:8050](http://<IP>:8050)

 Links for development:
 - https://webmasters.stackexchange.com/questions/44936/change-the-static-ip-address-to-domain-name
 - https://serverfault.com/questions/1021700/associate-static-ip-address-to-a-user-friendly-name
 - https://ybshankar010.medium.com/setting-up-your-django-website-with-gunicorn-nginx-and-godaddy-f53dfcec05df
 - https://www.digitalocean.com/community/tutorials/how-to-serve-flask-applications-with-gunicorn-and-nginx-on-ubuntu-22-04