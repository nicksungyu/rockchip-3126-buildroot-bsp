all:
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(LDFLAGS) -DPORTRAIT -D_DEBUG -DGAME_PACMANIA -o menu menu_Pacmania.cpp -lSDL2 -lSDL2_image -lSDL2_ttf -lpthread -lSDL2_mixer -lasound -liw -lusb -lusbpp
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(LDFLAGS) -o fingerprint fingerprint.cpp -lz
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(LDFLAGS) -o showImage showImage.c -lSDL2 -lSDL2_image
	@mkdir -p $(TARGET_DIR)/usr/share
	@mkdir -p $(TARGET_DIR)/usr/share/menu
	@mkdir -p $(TARGET_DIR)/usr/share/menu/images
	cp -R font $(TARGET_DIR)/usr/share/menu/.
	cp -R sound $(TARGET_DIR)/usr/share/menu/.
	#cp -R images $(TARGET_DIR)/usr/share/menu/.
	cp -Rf images_4_3/* $(TARGET_DIR)/usr/share/menu/images/.
	cp -Rf sound/intro.wav $(TARGET_DIR)/tmp
	./product.sh
	./game_update.sh
