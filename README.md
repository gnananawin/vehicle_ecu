1. git --version
2. make a directory and go inside it
3. git clone https://github.com/gnananawin/vehicle_ecu.git
4. git branch - make sure u are in main branch
5. code . - for opening it in VSC/ if not you can use terminal itself

   Running the Project:
1.make clean(This should be done in /home/your folder/vehicle_ecu/vehicle_ecu)
  make
2.use +chmod +x vehicle_ecu (if required)
3. ./vehicle_ecu (automated running)
4. ./vehicle_ecu -i (for giving manual cases)

  Making your changes:
After you make your changes in the directory follow the below:
1.git status
2.git add .
3.git commit -m "Initial local setup"
4.git push origin main
