#!/usr/bin/python2 -u

import logging

from datetime import datetime, timedelta

from flask import Flask, render_template, session, request, redirect, url_for

import door_lib
from ldap_user.flipdotuser import FlipdotUser
from ldap_user.webapp import FrontendError
import ldap_user.config as ldap_config
import config

logging.basicConfig(level=logging.INFO)

app = Flask(__name__, )

@app.route('/')
def index():
    if 'uid' not in session:
        return render_template('login.html')
    else:
        open, open_raw = is_door_open()
        return render_template('door.html',
            state="Open" if open else "Closed",
            state_raw=open_raw)

@app.route('/login', methods=['POST'])
def login():
    if request.method == 'POST':
        uid = request.form.get('uid', '')
        pwd = request.form.get('password', '')
        if not uid or not pwd:
            return redirect("/", 302)
        try:
            ldap = FlipdotUser()
            valid, dn = ldap.login(uid, pwd)
        except FrontendError as e:
            return render_template("error.html", message=e.message)
        if valid:
            session['uid'] = dn
            session['user'] = uid
            session['pass'] = pwd
        else:
            session.pop('uid', None)
        return redirect(url_for('index'))
    else:
        return redirect(url_for('index'))

@app.route('/logout')
def logout():
    session.clear()
    return redirect(url_for('index'))

@app.route('/door', methods=['POST'])
def door():
    if request.form.get('action', 'closedoor') == 'opendoor':

        if 'uid' not in session:
            return render_template('login.html')
        dn, user = FlipdotUser().getuser(session['uid'])
        if 'is_member' not in user['meta'] or not user['meta']['is_member']:
            return render_template('login.html')

        logging.info("Opening door.")
        door_lib.open()
    else:
        logging.info("Closing door.")
        door_lib.close()
    global door_time
    door_time = None
    return redirect(url_for('index'))

cache_time = timedelta(seconds=10)
door_time = None
door_open = None
def is_door_open():
    global door_open, door_time
    if door_open and door_time and door_time + cache_time > datetime.utcnow():
        return door_open

    with door_lib.get_serial() as s:
        door_open = door_lib.get_state(s)
    door_time = datetime.utcnow()
    return door_open

if __name__ == '__main__':
    app.secret_key = ldap_config.SECRET
    app.run(port=config.PORT, debug=True, )